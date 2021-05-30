/* -*- c++ -*- */
/*
 * Copyright 2021 gr-tempest
 *    Pablo Bertrand    <pablo.bertrand@fing.edu.uy>
 *    Felipe Carrau     <felipe.carrau@fing.edu.uy>
 *    Victoria Severi   <maria.severi@fing.edu.uy>
 *    
 *    Instituto de Ingeniería Eléctrica, Facultad de Ingeniería,
 *    Universidad de la República, Uruguay.
 *  
 * This is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3, or (at your option)
 * any later version.
 *  
 * This software is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *  
 * You should have received a copy of the GNU General Public License
 * along with this software; see the file COPYING.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street,
 * Boston, MA 02110-1301, USA.
 *
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <math.h>
#include <gnuradio/io_signature.h>
#include "frame_drop_impl.h"
#include <volk/volk.h>
#include <random>

namespace gr 
{
  namespace tempest 
  {

    frame_drop::sptr
    frame_drop::make(int Htotal, int Vtotal, int correct_sampling, float max_deviation, float update_proba, double actual_samp_rate)
    {
      return gnuradio::get_initial_sptr
        (new frame_drop_impl(Htotal, Vtotal, correct_sampling, max_deviation, update_proba, actual_samp_rate));
    }

    /*
     * The private constructor
     */
    frame_drop_impl::frame_drop_impl(int Htotal, int Vtotal, int correct_sampling, float max_deviation, float update_proba, double actual_samp_rate)
      : gr::block("frame_drop",
              gr::io_signature::make(1, 1, sizeof(gr_complex)),
              gr::io_signature::make(1, 1, sizeof(gr_complex))),
      d_dist(update_proba),
      d_gen(std::random_device{}()) 
    {
      set_relative_rate(1);

      //Counters
      d_sample_counter = 0; 
      d_frames_counter = 0;

      //Fixed values
      d_discarded_amount_per_frame = 3;

      //d_correct_sampling = correct_sampling; 
      d_proba_of_updating = update_proba;

      d_max_deviation = max_deviation;

      d_alpha_samp_inc = 1e-1;
      d_last_freq = 0;
      //d_actual_samp_rate = actual_samp_rate;
      d_samp_phase = 0; 
      d_alpha_corr = 1e-6; 

      const int alignment_multiple = volk_get_alignment() / sizeof(gr_complex);
      set_alignment(std::max(1, alignment_multiple));

      //set_output_multiple(d_Htotal);
      
      d_Htotal = Htotal; 
      d_Vtotal = Vtotal; 
      d_required_for_interpolation = d_Htotal*d_Vtotal;
      //d_max_deviation = max_deviation; 
      d_max_deviation_px = (int)std::ceil(d_Htotal*d_max_deviation);

      set_history(d_Vtotal*(d_Htotal+d_max_deviation_px)+1);

      d_peak_line_index = 0;
      d_samp_inc_rem = 0;
      d_new_interpolation_ratio_rem = 0;

      d_current_line_corr = new gr_complex[2*d_max_deviation_px + 1];
      d_historic_line_corr = new gr_complex[2*d_max_deviation_px + 1];
      d_abs_historic_line_corr = new float[2*d_max_deviation_px + 1];

      d_current_frame_corr = new gr_complex[2*(d_max_deviation_px+1)*d_Vtotal + 1];
      d_historic_frame_corr = new gr_complex[2*(d_max_deviation_px+1)*d_Vtotal + 1];
      d_abs_historic_frame_corr = new float[2*(d_max_deviation_px+1)*d_Vtotal + 1];

      d_next_update = 0;

      for (int i = 0; i<2*d_max_deviation_px+1; i++)
      {
        d_historic_line_corr[i] = 0;
        d_abs_historic_line_corr[i] = 0;
      }
      for (int i = 0; i<2*d_max_deviation_px*d_Vtotal+1; i++)
      {
        d_historic_frame_corr[i] = 0;
        d_abs_historic_frame_corr[i] = 0;
      }

    }

    /*
     * Our virtual destructor.
     */
    frame_drop_impl::~frame_drop_impl()
    {
      delete [] d_current_line_corr;
      delete [] d_historic_line_corr;
      delete [] d_abs_historic_line_corr;
      delete [] d_current_frame_corr;
      delete [] d_historic_frame_corr;
      delete [] d_abs_historic_frame_corr;  
    }


    void
    frame_drop_impl::forecast (int noutput_items, gr_vector_int &ninput_items_required)
    {
      int ninputs = ninput_items_required.size ();
      for (int i = 0; i < ninputs; i++){
        ninput_items_required[i] = (2*d_Htotal + 1)*(noutput_items/(2*d_Htotal) +1);
      }
    }


    void
    frame_drop_impl::estimate_peak_line_index(const gr_complex * in, int in_size)
    {

      gr_complex * d_in_conj = new gr_complex[in_size]; 
      volk_32fc_conjugate_32fc(&d_in_conj[0], &in[0], in_size);

      for (int i=0; i<in_size; i++)
      {
        volk_32fc_s32fc_multiply_32fc(&d_current_line_corr[0], &in[i+d_Htotal-d_max_deviation_px], d_in_conj[i], 2*d_max_deviation_px+1);
        volk_32fc_s32fc_multiply_32fc(&d_historic_line_corr[0], &d_historic_line_corr[0], (1-d_alpha_corr), 2*d_max_deviation_px+1);
        volk_32fc_x2_add_32fc(&d_historic_line_corr[0], &d_historic_line_corr[0], &d_current_line_corr[0], 2*d_max_deviation_px+1);
        volk_32fc_magnitude_squared_32f(&d_abs_historic_line_corr[0], &d_historic_line_corr[0], 2*d_max_deviation_px+1);
      }

      uint16_t peak_index = 0;
      uint32_t d_datain_length = (uint32_t)(2*d_max_deviation_px+1);
      volk_32f_index_max_16u(&peak_index, d_abs_historic_line_corr, 2*d_max_deviation_px+1); 

      d_peak_line_index = (peak_index-d_max_deviation_px);
      delete [] d_in_conj;
    }


    void 
    frame_drop_impl::update_interpolation_ratio(const gr_complex * in, int in_size)
    {
      gr_complex * d_in_conj = new gr_complex[in_size]; 
      volk_32fc_conjugate_32fc(&d_in_conj[0], &in[0], in_size);

      int corrsize = 2*d_Vtotal+1;
      int offset = (d_max_deviation_px+d_peak_line_index)*d_Vtotal;
      int offset_in = d_peak_line_index*d_Vtotal;
      for (int i=0; i<in_size; i++)
      {
        volk_32fc_s32fc_multiply_32fc(&d_current_frame_corr[offset], &in[i+d_Htotal*d_Vtotal+offset_in-d_Vtotal], d_in_conj[i], corrsize);
        volk_32fc_s32fc_multiply_32fc(&d_historic_frame_corr[offset], &d_historic_frame_corr[offset], (1-d_alpha_corr), corrsize);
        volk_32fc_x2_add_32fc(&d_historic_frame_corr[offset], &d_historic_frame_corr[offset], &d_current_frame_corr[offset], corrsize);
        volk_32fc_magnitude_squared_32f(&d_abs_historic_frame_corr[offset], &d_historic_frame_corr[offset], corrsize);
      }
      uint16_t peak_index = 0;
      uint32_t d_datain_length = (uint32_t)(corrsize);
      volk_32f_index_max_16u(&peak_index, &d_abs_historic_frame_corr[offset], corrsize); 

      d_new_interpolation_ratio_rem = ((double)(peak_index+offset_in-d_Vtotal))/(double)(d_Vtotal*d_Htotal);
      //printf("Some historics: %f\t %f\t %f\t %f\t \n",d_abs_historic_frame_corr[200],d_abs_historic_frame_corr[300],d_abs_historic_frame_corr[400],d_abs_historic_frame_corr[500]);
      delete [] d_in_conj;
    }


    void 
    frame_drop_impl::get_required_samples()
    {
      int ii = 0, oo = 0, incr;
      double s, f;
      
      d_samp_inc_rem = (1-d_alpha_samp_inc)*d_samp_inc_rem + d_alpha_samp_inc*d_new_interpolation_ratio_rem;

      while(oo < d_Htotal*d_Vtotal) {
        s = d_samp_phase + d_samp_inc_rem + 1;
        f = floor(s);
        incr = (int)f;
        d_samp_phase = s - f;
        ii += incr;
        oo++;
      }
      d_required_for_interpolation = ii;
    }


    int
    frame_drop_impl::general_work (int noutput_items,
                       gr_vector_int &ninput_items,
                       gr_vector_const_void_star &input_items,
                       gr_vector_void_star &output_items)
    {
      const gr_complex *in = (const gr_complex *) input_items[0];
      gr_complex *out = (gr_complex *) output_items[0];

      int consumed = 0, out_amount = 0;
      
      ////////////////////////////////////////////////////////////
     
      d_next_update -= noutput_items;

      if(d_next_update <= 0){

        estimate_peak_line_index(&in[0], noutput_items);
        update_interpolation_ratio(&in[0], noutput_items);

        if (d_next_update <= -10 * d_Htotal){
          d_next_update = d_dist(d_gen);
        }
      }
     
      ///////////////////////////////////////////////////////////

      /*for (int i=0; i<noutput_items; i++){

        d_sample_counter++;

        if (d_sample_counter <= d_required_for_interpolation){

          out[i]=in[i];
          out_amount++;

        } else if (d_sample_counter == (d_discarded_amount_per_frame*d_required_for_interpolation)){

          d_sample_counter = 0;
          get_required_samples();

        }  
      }

      consumed += noutput_items;*/

      ///////////////////////////////////////////////////////////
      d_state = get_state(noutput_items);

      switch (d_state)
      {
        case State_e::idle:
        case State_e::case_discard:
          {
            consumed += noutput_items;
            d_sample_counter += noutput_items;
          }
          break;

        case State_e::case_display:
          {
            consumed += noutput_items;
            out_amount += noutput_items;
            memcpy(&out[0], &in[0], noutput_items*sizeof(gr_complex));
            d_sample_counter += noutput_items;
          }
          break;

        case State_e::case_frame_end_from_display_to_discard:
          { 
            //Process, finish displaying until d_required_for_interpolation then reset counter 
            int display_frame_end_samples = d_required_for_interpolation - d_sample_counter;

            memcpy(&out[0], &in[0], display_frame_end_samples*sizeof(gr_complex));   
            d_sample_counter += display_frame_end_samples;    
            
            consumed += noutput_items;
            out_amount += display_frame_end_samples;

            // End state:
            if(d_sample_counter == d_required_for_interpolation)
            {
              d_sample_counter = noutput_items - display_frame_end_samples;
              d_frames_counter = 0;
              get_required_samples();
            }
          }
          break;

        case State_e::case_frame_end_from_discard_to_discard:
          {
            
            // Process, finish discarding elements until d_required_for_interpolation 
            int discard_frame_end_samples = d_required_for_interpolation - d_sample_counter;
            d_sample_counter += discard_frame_end_samples;   

            consumed += noutput_items;

            // End state:
            if(d_sample_counter == d_required_for_interpolation)
            {
              d_sample_counter = noutput_items - discard_frame_end_samples;
              d_frames_counter ++;
            }

          }
          break;

        case State_e::case_frame_end_from_discard_to_display:
          {
            
            //  Process, finish discarding elements start displaying elements until d_required_for_interpolation
            int discard_frame_end_samples = d_required_for_interpolation - d_sample_counter;
            d_sample_counter += discard_frame_end_samples;   

            consumed += noutput_items;

            // End state:
            if(d_sample_counter == d_required_for_interpolation)
            {
              d_sample_counter = noutput_items - discard_frame_end_samples;

              int display_frame_start_samples = d_sample_counter;
              memcpy(&out[0], &in[0], display_frame_start_samples*sizeof(gr_complex));

              out_amount += display_frame_start_samples;  

              d_frames_counter ++;
            }
            
          }
          break;

        default:
          break;
      }
      // Tell runtime system how many input items we consumed on
      // each input stream.
      consume_each (consumed);

      // Tell runtime system how many output items we produced.
      return out_amount;
    
    }    
        frame_drop_impl::State_e frame_drop_impl::get_state(int noutput_items)
    {
        if(
            (d_sample_counter + noutput_items <= d_required_for_interpolation) 
            && (d_frames_counter < d_discarded_amount_per_frame)
          )
        {
          return State_e::case_discard;
        }

        if(
            (d_sample_counter + noutput_items <= d_required_for_interpolation) 
            && (d_frames_counter == d_discarded_amount_per_frame)
          )
        {
          return State_e::case_display;
        }

        if(
            (d_sample_counter + noutput_items >= d_required_for_interpolation) 
            && (d_frames_counter == d_discarded_amount_per_frame)
          )
        {
          return State_e::case_frame_end_from_display_to_discard;
        }

        if(
            (d_sample_counter + noutput_items >= d_required_for_interpolation) 
            && (d_frames_counter < d_discarded_amount_per_frame - 1)
          )
        {
          return State_e::case_frame_end_from_discard_to_discard;
        }

        if(
            (d_sample_counter + noutput_items >= d_required_for_interpolation) 
            && (d_frames_counter == d_discarded_amount_per_frame - 1)
          )
        {
          return State_e::case_frame_end_from_discard_to_display;
        }
        
        //Not intended state. Something is wrong.   
        return State_e::idle;
    }
    
  } /* namespace tempest */
} /* namespace gr */


