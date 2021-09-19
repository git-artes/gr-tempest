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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <gnuradio/io_signature.h>
#include "fft_peak_fine_sampling_sync_impl.h"
#include <thread>
#include <volk/volk.h>
#include <math.h>

#define N 8192


namespace gr {
  namespace tempest {

    fft_peak_fine_sampling_sync::sptr
    fft_peak_fine_sampling_sync::make(int sample_rate, int size, int refresh_rate, int Vvisible, int Hvisible, bool automatic_mode)
    {
      return gnuradio::get_initial_sptr
        (new fft_peak_fine_sampling_sync_impl(sample_rate, size, refresh_rate, Vvisible, Hvisible, automatic_mode));
    }


    /*
     * The private constructor
     */
    fft_peak_fine_sampling_sync_impl::fft_peak_fine_sampling_sync_impl(int sample_rate, int size, int refresh_rate, int Vvisible, int Hvisible, bool automatic_mode)
      : gr::block("fft_peak_fine_sampling_sync",
              gr::io_signature::make(1, 1, sizeof(float)),
              gr::io_signature::make(1, 1, sizeof(float)))
    {
      d_start_fft_peak_finder = 1;
      //Received parameters
      d_sample_rate = sample_rate;
      d_fft_size = size;

      //Search values
      d_search_skip = 0;
      d_search_margin = d_fft_size;
      d_vtotal_est = 0;
      
      //Parameters to publish
      d_refresh_rate = 0;
      d_Hvisible = Hvisible;
      d_Vvisible = Vvisible;
      d_Hblank = 0;
      d_Vblank = 0;

      d_start = true;

      d_ratio = 0;
      d_accumulator = 0.0f;
      //d_real_line = 827;  
      d_real_line = 827.076923077;  


      //Counters
      d_work_counter = 1;
      
      //PMT ports
      message_port_register_out(pmt::mp("en"));
      message_port_register_out(pmt::mp("ratio"));
      message_port_register_out(pmt::mp("smpl"));

      message_port_register_in(pmt::mp("en"));

      // PMT handlers
      set_msg_handler(pmt::mp("en"),   [this](const pmt::pmt_t& msg) {fft_peak_fine_sampling_sync_impl::set_ena_msg(msg); });

      set_history(d_fft_size);
    }

    /*
     * Our virtual destructor.
     */
    fft_peak_fine_sampling_sync_impl::~fft_peak_fine_sampling_sync_impl()
    {
    }

    void 
    fft_peak_fine_sampling_sync_impl::set_ena_msg(pmt::pmt_t msg)
    {
        gr::thread::scoped_lock l(d_mutex); 
        if (pmt::is_bool(msg)) {
            bool en = pmt::to_bool(msg);
            d_start_fft_peak_finder = !en;
            printf("FFT peak finder. Ratio calculation stopped.\n");
        } else {
            GR_LOG_WARN(d_logger,
                        "FFT peak finder: Non-PMT type received, expecting Boolean PMT\n");
            d_start_fft_peak_finder = 0;
            printf("FFT peak finder. Ratio calculation stopped.\n");
        }
    }


    void
    fft_peak_fine_sampling_sync_impl::forecast (int noutput_items, gr_vector_int &ninput_items_required)
    {
      /* <+forecast+> e.g. ninput_items_required[0] = noutput_items */
    }

    int
    fft_peak_fine_sampling_sync_impl::general_work (int noutput_items,
                       gr_vector_int &ninput_items,
                       gr_vector_const_void_star &input_items,
                       gr_vector_void_star &output_items)
    {
      const float *in = (const float *) input_items[0];
      float *out = (float *) output_items[0];

      gr::thread::scoped_lock l(d_mutex);
                                                             /* Work iteration counter */
      if(d_start_fft_peak_finder)
      {

          /////////////////////////////
          //      RATIO SEARCH       //
          /////////////////////////////

          uint32_t peak_index = 0, peak_index_2 = 0, yt_index = 0, yt_aux = 0;
          
          d_search_skip= 0;
          volk_32f_index_max_32u(&peak_index, &in[d_search_skip], floor(d_search_margin) );   /* 'descartados' se elige para que de cerca del pico conocido */
          
          //d_search_skip= round(0);
          //volk_32f_index_max_32u(&peak_index, &in[0], floor(d_search_margin) );   /* 'descartados' se elige para que de cerca del pico conocido */

          peak_index += d_search_skip;                     
                                                   /* Intentar que varíe menos que fv */
          add_item_tag(0, nitems_written(0) + peak_index, pmt::mp("peak_1"), pmt::PMT_T); 

          ////////////////////////////////////////////////////////////////////////////// Second peak
          uint32_t one_full_frame_in_samples=floor((0.0166656)*d_sample_rate);

          d_search_skip = peak_index + one_full_frame_in_samples - floor((0.001)*d_sample_rate);
          int search_range = 200 + floor((0.001)*5*d_sample_rate);//floor(d_fft_size/2) - d_search_skip;// use dHtotal
          
          volk_32f_index_max_32u(&peak_index_2, &in[d_search_skip], search_range);   /* 'descartados' se elige para que de cerca del pico conocido */

          peak_index_2 += d_search_skip;                                                /* Offset por indice relativo en volk */

          add_item_tag(0, nitems_written(0) + peak_index_2, pmt::mp("peak_2"), pmt::PMT_T); 
          // 827 826 827 // el acumulador deberia dar 829.7053735

          if(1)// (peak_index_2-peak_index) > 500 )
            d_accumulator += (long double)(peak_index_2-peak_index)/(long double)(N);
          else
            d_accumulator += (long double)(d_real_line*d_Vvisible)/(long double)(N);

          if(d_work_counter%N == 0)
          {
            // Compare with:
            //printf("d_search_skip %d d_search_margin  %d \t\n", d_search_skip, d_search_margin);

            long double line_timing = (long double)(d_accumulator)/(long double)d_sample_rate;
            long double ratio_timings = line_timing*1000000 / (long double)(16.6656*1000);
            //long double ratio = (long double)(d_accumulator)/(long double)(d_real_line*d_Vvisible);
            long double ratio = (long double)(d_accumulator)/(long double)(d_Hvisible*d_Vvisible);

            d_ratio = (ratio-1);
            //d_ratio = (0.01f)*((double)ratio-1.0f) + (1-0.01f)*d_ratio; 

            /* Add Tag. */
            double new_freq = (double)d_ratio;

            message_port_pub(
                          pmt::mp("ratio"), 
                          pmt::cons(pmt::mp("ratio"), pmt::from_double(new_freq))
                        );
            message_port_pub(
                          pmt::mp("smpl"), 
                          pmt::cons(pmt::mp("smpl"), pmt::from_double(d_accumulator))
                        );


            d_accumulator = 0;
            d_work_counter = 0;

            printf("Line timing \t %Lf us. \t Ratio = \t %f  RatioTimings = \t %Lf  \r\n ", line_timing*1000000, new_freq, ratio_timings);    
            //printf("Peaks delta \t %Lf \t \t\r\n ", d_accumulator);  
            if( ratio_timings > 0.9997 && ratio_timings < 1.000162   ){
            //if( ratio_timings > 0.99 && ratio_timings < 1.005   ){
                bool bool_msg = false;
                message_port_pub(
                          pmt::mp("en"), 
                          pmt::from_bool(bool_msg)
                        );
                /*
                message_port_pub(
                          pmt::mp("ratio"), 
                          pmt::cons(pmt::mp("ratio"), pmt::from_double(new_freq))
                        );
                */
                /* 
                  Stop fine sampling synchronization and sleep for a long period.
                      - Commented. Because this stops execution of the entire flowgraph, somehow.

                */
                long period_ms = (100000);
                boost::this_thread::sleep(  boost::posix_time::milliseconds(static_cast<long>(period_ms)) );
                //return WORK_DONE;
            } else{
                //Sleep for short period of time.. this affects the entire flowgraph.
                long period_ms = (500);
                //boost::this_thread::sleep(  boost::posix_time::milliseconds(static_cast<long>(period_ms)) );
            }

            
          }
          d_work_counter++;   
          //memcpy(out, in, noutput_items*sizeof(float));
          // Tell runtime system how many input items we consumed on
          // each input stream.
          consume_each (noutput_items);

          // Tell runtime system how many output items we produced.
          return noutput_items; 
      } else
      {
          return WORK_DONE;
      }
    }

  } /* namespace tempest */
} /* namespace gr */

