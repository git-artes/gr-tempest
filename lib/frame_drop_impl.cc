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
#include <vector>

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
      d_inter(gr::filter::mmse_fir_interpolator_cc()),
      d_dist(update_proba),
      d_gen(std::random_device{}()) 
    {
      set_relative_rate(1);

      //Counters
      d_sample_counter = 0; 
      d_display_counter = 0;
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
      
      d_Htotal = Htotal; 
      d_Vtotal = Vtotal; 
      d_required_for_interpolation = d_Htotal*d_Vtotal;
      //d_max_deviation = max_deviation; 
      d_max_deviation_px = (int)std::ceil(d_Htotal*d_max_deviation);

      set_history(d_Vtotal*(d_Htotal+d_max_deviation_px)+1);

      d_peak_line_index = 0;
      d_samp_inc_rem = 0;
      d_new_interpolation_ratio_rem = 0;
      d_next_update = 0;

      /* Volk_Malloc
        https://github.com/gnuradio/volk/blob/master/lib/volk_malloc.c
      */

      const int alignment_multiple = volk_get_alignment() / sizeof(gr_complex);
      set_alignment(std::max(1, alignment_multiple));

      //set_output_multiple(d_Htotal);

      /* 
        Complex type arrays aligned memory allocations:
      */
      d_current_line_corr =       (gr_complex*)volk_malloc((2*d_max_deviation_px + 1)              *sizeof(gr_complex), volk_get_alignment() / sizeof(gr_complex));
      d_historic_line_corr =      (gr_complex*)volk_malloc((2*d_max_deviation_px + 1)              *sizeof(gr_complex), volk_get_alignment() / sizeof(gr_complex));
      d_current_frame_corr =      (gr_complex*)volk_malloc((2*(d_max_deviation_px+1)*d_Vtotal + 1) *sizeof(gr_complex), volk_get_alignment() / sizeof(gr_complex));
      d_historic_frame_corr =     (gr_complex*)volk_malloc((2*(d_max_deviation_px+1)*d_Vtotal + 1) *sizeof(gr_complex), volk_get_alignment() / sizeof(gr_complex));

      /* 
        Alignment per Block of malloc / type necessary? Floats
      const int float_alignment = volk_get_alignment() / sizeof(float);
      set_alignment(std::max(1, float_alignment));
      */      
      d_abs_historic_line_corr =  (float*)volk_malloc((2*d_max_deviation_px + 1)              *sizeof(float),     volk_get_alignment() / sizeof(float));
      d_abs_historic_frame_corr = (float*)volk_malloc((2*(d_max_deviation_px+1)*d_Vtotal + 1) *sizeof(float),     volk_get_alignment() / sizeof(float));

      /* 
        Alignment per Block of malloc / type necessary? Back to complex or uint32_t:
      const int float_alignment = volk_get_alignment() / sizeof(gr_complex);
      set_alignment(std::max(1, float_alignment));
      */
      
      /*
      d_input_index = new uint32_t[d_Htotal*d_Vtotal];
      d_historic_samp_phase = new double[d_Htotal*d_Vtotal];
      memset(&d_input_index[0],               0,  d_Htotal*d_Vtotal);
      memset(&d_historic_samp_phase[0],       0,  d_Htotal*d_Vtotal);
      */

      /*
      memset(&d_current_line_corr[0],         0,  2*d_max_deviation_px+1);
      memset(&d_historic_line_corr[0],        0,  2*d_max_deviation_px+1);
      memset(&d_abs_historic_line_corr[0],    0,  2*d_max_deviation_px+1);
      memset(&d_current_frame_corr[0],        0,  2*d_max_deviation_px*d_Vtotal+1);
      memset(&d_historic_frame_corr[0],       0,  2*d_max_deviation_px*d_Vtotal+1);
      memset(&d_abs_historic_frame_corr[0],   0,  2*d_max_deviation_px*d_Vtotal+1);
      */

      memset(&d_current_line_corr[0],         0,  sizeof(gr_complex)*(2*d_max_deviation_px+1));
      memset(&d_historic_line_corr[0],        0,  sizeof(gr_complex)*(2*d_max_deviation_px+1));
      memset(&d_abs_historic_line_corr[0],    0,  sizeof(float)*(2*d_max_deviation_px+1));
      memset(&d_current_frame_corr[0],        0,  sizeof(gr_complex)*(2*d_max_deviation_px*d_Vtotal+1));
      memset(&d_historic_frame_corr[0],       0,  sizeof(gr_complex)*(2*d_max_deviation_px*d_Vtotal+1));
      memset(&d_abs_historic_frame_corr[0],   0,  sizeof(float)*(2*d_max_deviation_px*d_Vtotal+1));

      // PMT port
      message_port_register_out(pmt::mp("ratio"));

    }

    /*
     * Our virtual destructor.
     */
    frame_drop_impl::~frame_drop_impl()
    {
      volk_free(d_input_index);
      volk_free(d_historic_samp_phase);
      volk_free(d_current_line_corr);
      volk_free(d_historic_line_corr);
      volk_free(d_abs_historic_line_corr);
      volk_free(d_current_frame_corr);
      volk_free(d_historic_frame_corr);
      volk_free(d_abs_historic_frame_corr);
    }


    void
    frame_drop_impl::forecast (int noutput_items, gr_vector_int &ninput_items_required)
    {
        int ninputs = ninput_items_required.size ();
        for (int i = 0; i < ninputs; i++)
        {
            ninput_items_required[i] = (int)ceil((noutput_items + 1) * (2+d_samp_inc_rem)) + d_inter.ntaps() ;
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


    void 
    frame_drop_impl::get_required_samples(int size)
    {
      uint32_t ii = 0, incr;
      int oo = 0;
      double s, f;
      
      d_samp_inc_rem = (1-d_alpha_samp_inc)*d_samp_inc_rem + d_alpha_samp_inc*d_new_interpolation_ratio_rem;

      while(oo < size) {
        s = d_samp_phase + d_samp_inc_rem + 1;
        f = floor(s);
        incr = (uint32_t)f;
        d_samp_phase = s - f;
        ii += incr;

        //d_input_index[oo] = ii;
        //d_historic_samp_phase[oo] = d_samp_phase;

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

      int consumed = 0, out_amount = 0, aux;
      
      ////////////////////////////////////////////////////////////
     
      d_next_update -= noutput_items;

      if(d_next_update <= 0){

        estimate_peak_line_index(&in[0], noutput_items);
        update_interpolation_ratio(&in[0], noutput_items);

        double new_freq = d_new_interpolation_ratio_rem;

        message_port_pub(
          pmt::mp("ratio"), 
          pmt::cons(
            pmt::mp("ratio"), 
            pmt::from_double(new_freq)
          )
        );

        if (d_next_update <= -10 * d_Htotal){
          d_next_update = d_dist(d_gen);
        }
      }
     
      ///////////////////////////////////////////////////////////

      for (int i=0; i<noutput_items; i++){

        d_sample_counter++;

        if (d_sample_counter <= d_required_for_interpolation){

          out[i]=in[i];
          out_amount++;

        } else if (d_sample_counter == (d_discarded_amount_per_frame*d_required_for_interpolation)){

          d_sample_counter = 0;
          get_required_samples(d_Htotal*d_Vtotal);

        }
      }

      consumed += noutput_items;

      // Tell runtime system how many input items we consumed on
      // each input stream.
      consume_each (consumed);

      // Tell runtime system how many output items we produced.
      return out_amount;
    
    }    
    
  } /* namespace tempest */
} /* namespace gr */


