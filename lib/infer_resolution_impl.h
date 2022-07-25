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

#ifndef INCLUDED_TEMPEST_INFER_RESOLUTION_IMPL_H
#define INCLUDED_TEMPEST_INFER_RESOLUTION_IMPL_H

#include <tempest/infer_resolution.h>

namespace gr {
  namespace tempest {

    class infer_resolution_impl : public infer_resolution
    {
     private:
      // Nothing to declare in this block.

     public:
      infer_resolution_impl(int samp_rate, int fft_size);
      ~infer_resolution_impl();

      //Received parameters
      int d_sample_rate;
      int d_fft_size;

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

      //Functions
      void publish_messages();
      void search_table(double fv_estimated);

      // Where all the action really happens
      void forecast (int noutput_items, gr_vector_int &ninput_items_required);

      int general_work(int noutput_items,
           gr_vector_int &ninput_items,
           gr_vector_const_void_star &input_items,
           gr_vector_void_star &output_items);

    };

  } // namespace tempest
} // namespace gr

#endif /* INCLUDED_TEMPEST_INFER_RESOLUTION_IMPL_H */

