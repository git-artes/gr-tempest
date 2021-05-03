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
      d_frame_height_counter = 0; 
      d_frames_counter = 0;

      //Fixed values
      d_discarded_amount_per_frame = 3;

      d_correct_sampling = correct_sampling; 
      d_proba_of_updating = update_proba;

      d_max_deviation = max_deviation;

      d_alpha_samp_inc = 1e-1;
      d_last_freq = 0;
      d_actual_samp_rate = actual_samp_rate;
      d_samp_phase = 0; 
      d_alpha_corr = 1e-6; 

      const int alignment_multiple = volk_get_alignment() / sizeof(gr_complex);
      set_alignment(std::max(1, alignment_multiple));

      //set_output_multiple(d_Htotal);
      
      d_Htotal = Htotal; 
      d_Vtotal = Vtotal; 
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
      delete [] d_in_conj;
    }


    int
    frame_drop_impl::general_work (int noutput_items,
                       gr_vector_int &ninput_items,
                       gr_vector_const_void_star &input_items,
                       gr_vector_void_star &output_items)
    {
      const gr_complex *in = (const gr_complex *) input_items[0];
      gr_complex *out = (gr_complex *) output_items[0];

      int consumed = 0, out_amount = 0, required_for_interpolation = noutput_items, ii = 0, oo = 0, incr;
      double s, f;

      d_next_update -= noutput_items;

      if(d_next_update <= 0){

        estimate_peak_line_index(in, noutput_items);
        update_interpolation_ratio(in, noutput_items);

        if (d_next_update <= -10 * d_Htotal){
          d_next_update = d_dist(d_gen);
        }
      }

      if (d_correct_sampling){
        
        d_samp_inc_rem = (1-d_alpha_samp_inc)*d_samp_inc_rem + d_alpha_samp_inc*d_new_interpolation_ratio_rem;
        
        while(oo < noutput_items) {
          s = d_samp_phase + d_samp_inc_rem + 1;
          f = floor(s);
          incr = (int)f;
          d_samp_phase = s - f;
          ii += incr;
          oo++;
        }

        required_for_interpolation = ii;
      }

      for (int line = 0; line < required_for_interpolation/d_Htotal; line++) 
      { 

        //If we are in one of the n discarded frames
        if (d_frames_counter < d_discarded_amount_per_frame)
        {
          //lines are counted and consumed to see them all through
          d_frame_height_counter ++;
          consumed += d_Htotal;                                             

          //and three frames are counted without any output
          if (d_frame_height_counter % d_Vtotal == 0)
          {
              d_frame_height_counter = 0;
              d_frames_counter++;
          }

          //If we are in the one frame we wish to keep
        } else if (d_frames_counter == d_discarded_amount_per_frame){

          //the data is copied in the output, consuming accordingly
          memcpy(&out[line*d_Htotal], &in[line*d_Htotal], d_Htotal*sizeof(gr_complex));
          out_amount += d_Htotal;
          consumed += d_Htotal;
          d_frame_height_counter ++;

          //until the frame is over, so we begin again
          if (d_frame_height_counter % d_Vtotal == 0)
          {
              d_frame_height_counter = 0;
              d_frames_counter = 0;
          }
        }
      }

      // Tell runtime system how many input items we consumed on
      // each input stream.
      consume_each (consumed);

      // Tell runtime system how many output items we produced.
      return out_amount;
    
    }    

  } /* namespace tempest */
} /* namespace gr */

