/* -*- c++ -*- */
/* 
 * Copyright 2020
 *   Federico "Larroca" La Rocca <flarroca@fing.edu.uy>
 * 
 *   Instituto de Ingenieria Electrica, Facultad de Ingenieria,
 *   Universidad de la Republica, Uruguay.
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

#ifndef INCLUDED_TEMPEST_FINE_SAMPLING_SYNCHRONIZATION_IMPL_H
#define INCLUDED_TEMPEST_FINE_SAMPLING_SYNCHRONIZATION_IMPL_H

#include <gnuradio/tempest/fine_sampling_synchronization.h>
#include <gnuradio/filter/mmse_fir_interpolator_cc.h>
#include <random>

namespace gr {
  namespace tempest {

    class fine_sampling_synchronization_impl : public fine_sampling_synchronization
    {
     private:
         int d_Htotal; 
         int d_Vtotal; 
         int d_correct_sampling;
         float d_max_deviation; 

         // to accelerate the process, I'll only update the interpolation
         // once every a random number of iterations with probability d_proba_of_updating
         std::geometric_distribution<int> d_dist;
         std::minstd_rand d_gen;
         float d_proba_of_updating;
         int d_next_update;

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
      
        // where is the line correlation peak  
         int d_peak_line_index;

         float d_alpha_corr;

         // the interpolating filter        
         gr::filter::mmse_fir_interpolator_cc d_inter; 

        int interpolate_input(const gr_complex * in, gr_complex * out, int size);
      
        void update_interpolation_ratio(const gr_complex * in, int in_size);

        void estimate_peak_line_index(const gr_complex * in, int in_size);

     public:
      fine_sampling_synchronization_impl(int Htotal, int Vtotal, int correct_sampling, float max_deviation, float update_proba);
      ~fine_sampling_synchronization_impl();

      void set_Htotal_Vtotal(int Htotal, int Vtotal);

      // Where all the action really happens
      void forecast (int noutput_items, gr_vector_int &ninput_items_required);

      int general_work(int noutput_items,
           gr_vector_int &ninput_items,
           gr_vector_const_void_star &input_items,
           gr_vector_void_star &output_items);
    };

  } // namespace tempest
} // namespace gr

#endif /* INCLUDED_TEMPEST_FINE_SAMPLING_SYNCHRONIZATION_IMPL_H */


