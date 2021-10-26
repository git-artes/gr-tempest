/* -*- c++ -*- */
/**
 * Copyright 2021
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
 * @file fft_peak_fine_sampling_sync_impl.h
 * 
 * @brief Block that uses the signal autocorrelation to
 * continuously calculate the interpolation ratio required,
 * for as long as it takes to properly correct sampling.
 *
 * gr-tempest
 *
 * @date September 19, 2021
 * @author  Pablo Bertrand   <pablo.bertrand@fing.edu.uy>
 * @author  Felipe Carrau    <felipe.carrau@fing.edu.uy>
 * @author  Victoria Severi  <maria.severi@fing.edu.uy>
 */

/**********************************************************
 * Constant and macro definitions
 **********************************************************/

#ifndef INCLUDED_TEMPEST_FFT_PEAK_FINE_SAMPLING_SYNC_IMPL_H
#define INCLUDED_TEMPEST_FFT_PEAK_FINE_SAMPLING_SYNC_IMPL_H

/**********************************************************
 * Include statements
 **********************************************************/

#include <tempest/fft_peak_fine_sampling_sync.h>

namespace gr {
  namespace tempest {

    class fft_peak_fine_sampling_sync_impl : public fft_peak_fine_sampling_sync
    {
     private:

      /**********************************************************
       * Data declarations
       **********************************************************/

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
      uint32_t d_peak_1;
      uint32_t d_peak_2;

      //Results to publish
      long d_refresh_rate;
      long d_Hvisible;
      long d_Vvisible;
      long d_Hblank;
      long d_Vblank;

      //Counters
      uint32_t d_work_counter;
      uint32_t d_sample_counter;

      /**********************************************************
       * Private function prototypes
       **********************************************************/
      /**
        * @brief Function that processes the enable received as PMT
        * message from other blocks and assignes it to a variable.
        *  
        * @param pmt_t msg: Message received from autocorrelation
        * block during runtime.
        */
      void set_ena_msg(pmt::pmt_t msg);
      //---------------------------------------------------------
     public:
      fft_peak_fine_sampling_sync_impl(int sample_rate, int size, int refresh_rate, int Vvisible, int Hvisible, bool automatic_mode);
      ~fft_peak_fine_sampling_sync_impl();

      /**********************************************************
       * Public function prototypes
       **********************************************************/
      /**
        * @brief Used to establish the amount of samples required
        * for a full work iteration.
        *  
        */
      void forecast (int noutput_items, gr_vector_int &ninput_items_required);
      //---------------------------------------------------------
      /**
        * @brief Main function that carries out the peak searches
        * in the autocorrelation signal and uses the results to
        * calculate the interpolation ratio required and prints 
        * it to a PMT port for other blocks to use it as it is 
        * updated in runtime.
        *  
        */
      int general_work(int noutput_items,
           gr_vector_int &ninput_items,
           gr_vector_const_void_star &input_items,
           gr_vector_void_star &output_items);
      //---------------------------------------------------------
    };

  } // namespace tempest
} // namespace gr

#endif /* INCLUDED_TEMPEST_FFT_PEAK_FINE_SAMPLING_SYNC_IMPL_H */

