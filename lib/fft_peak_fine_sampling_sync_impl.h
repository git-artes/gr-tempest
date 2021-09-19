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

#ifndef INCLUDED_TEMPEST_FFT_PEAK_FINE_SAMPLING_SYNC_IMPL_H
#define INCLUDED_TEMPEST_FFT_PEAK_FINE_SAMPLING_SYNC_IMPL_H

#include <tempest/fft_peak_fine_sampling_sync.h>

namespace gr {
  namespace tempest {

    class fft_peak_fine_sampling_sync_impl : public fft_peak_fine_sampling_sync
    {
     private:
      // Nothing to declare in this block.

      gr::thread::mutex d_mutex;
      bool d_start_fft_peak_finder = 1;

      //Received parameters
      int d_sample_rate;
      int d_fft_size;


      double d_ratio;
      long double d_accumulator;
      long double d_real_line;
      bool d_start;


      //Search values
      uint32_t d_search_skip;
      uint32_t d_search_margin;
      uint32_t d_vtotal_est;

      //Results to publish
      long d_refresh_rate;
      long d_Hvisible;
      long d_Vvisible;
      long d_Hblank;
      long d_Vblank;

      //Counters
      uint32_t d_work_counter;
      
      void set_ena_msg(pmt::pmt_t msg);

     public:
      fft_peak_fine_sampling_sync_impl(int sample_rate, int size, int refresh_rate, int Vvisible, int Hvisible, bool automatic_mode);
      ~fft_peak_fine_sampling_sync_impl();

      // Where all the action really happens
      void forecast (int noutput_items, gr_vector_int &ninput_items_required);

      int general_work(int noutput_items,
           gr_vector_int &ninput_items,
           gr_vector_const_void_star &input_items,
           gr_vector_void_star &output_items);

    };

  } // namespace tempest
} // namespace gr

#endif /* INCLUDED_TEMPEST_FFT_PEAK_FINE_SAMPLING_SYNC_IMPL_H */

