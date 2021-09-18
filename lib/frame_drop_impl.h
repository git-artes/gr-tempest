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

#ifndef INCLUDED_TEMPEST_FRAME_DROP_IMPL_H
#define INCLUDED_TEMPEST_FRAME_DROP_IMPL_H

#include <gnuradio/filter/mmse_fir_interpolator_cc.h>
#include <random>
#include <tempest/frame_drop.h>
#include <stdio.h>
#include <math.h>

namespace gr {
  namespace tempest {

    class frame_drop_impl : public frame_drop
    {
     private:

      //Counters
      //int d_frame_height_counter;
      int d_display_counter;
      int d_frames_counter;
      int d_sample_counter;
      
      //Fixed parameter
      int d_discarded_amount_per_frame;

      int d_Htotal; 
      int d_Vtotal; 
      int d_correct_sampling;
      uint32_t d_required_for_interpolation;
      float d_max_deviation;

      std::geometric_distribution<int> d_dist;
      std::minstd_rand d_gen;
      
      float d_proba_of_updating;
      int d_next_update;

      //double d_actual_samp_rate;
      double d_samp_inc_rem;
      double d_new_interpolation_ratio_rem;
      double d_samp_phase;

      float d_alpha_samp_inc; 
      int d_max_deviation_px; 

      // correlation with the last line 
      gr_complex * d_current_line_corr;  
      gr_complex * d_historic_line_corr;  
      float * d_abs_historic_line_corr;  
      // correlation with the last frame 
      gr_complex * d_current_frame_corr;  
      gr_complex * d_historic_frame_corr;  
      float * d_abs_historic_frame_corr;  

      int d_peak_line_index;

      float d_alpha_corr;
      double d_last_freq ;

      uint32_t * d_input_index;
      double * d_historic_samp_phase;

      bool d_start_frame_drop;

      // the interpolating filter        
      gr::filter::mmse_fir_interpolator_cc d_inter; 

      void update_interpolation_ratio(const gr_complex * in, int in_size);

      void estimate_peak_line_index(const gr_complex * in, int in_size);

      //void set_iHsize_msg(pmt::pmt_t msg);
      //void set_Vsize_msg(pmt::pmt_t msg);

      gr::thread::mutex d_mutex;
      void set_ena_msg(pmt::pmt_t msg);
      void set_smpl_msg(pmt::pmt_t msg);
      void set_ratio_msg(pmt::pmt_t msg);
     
     public:
      frame_drop_impl(int Htotal, int Vtotal, int correct_sampling, float max_deviation, float update_proba, double actual_samp_rate);
      ~frame_drop_impl();

      void forecast (int noutput_items, gr_vector_int &ninput_items_required);
      void get_required_samples(int size);

      int general_work(int noutput_items,
           gr_vector_int &ninput_items,
           gr_vector_const_void_star &input_items,
           gr_vector_void_star &output_items);

    };

  } // namespace tempest
} // namespace gr

#endif /* INCLUDED_TEMPEST_FRAME_DROP_IMPL_H */

