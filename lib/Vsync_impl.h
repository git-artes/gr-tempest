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

#ifndef INCLUDED_TEMPEST_VSYNC_IMPL_H
#define INCLUDED_TEMPEST_VSYNC_IMPL_H

#include <tempest/Vsync.h>

namespace gr {
  namespace tempest {

    class Vsync_impl : public Vsync
    {
     private:
         int d_Htotal; 
         int d_Vtotal;
         int d_Vblank;

         int d_delay;

         int d_lines_processed;

         gr_complex * d_energy_per_line;
         float * d_energy_avg;
         float d_alpha_energy_avg;

         int d_delta;

         void set_delay(int delay);
         int get_delay();

     public:
      Vsync_impl(int Htotal, int Vtotal, int Vblank);
      ~Vsync_impl();

      void forecast (int noutput_items, gr_vector_int &ninput_items_required);
      
      // Where all the action really happens
      int general_work(int noutput_items,
           gr_vector_int &ninput_items,
           gr_vector_const_void_star &input_items,
           gr_vector_void_star &output_items);


      void set_Htotal_and_Vtotal(int Htotal, int Vtotal);
      
      void set_Vblank(int Vblank);
    };


  } // namespace tempest
} // namespace gr

#endif /* INCLUDED_TEMPEST_VSYNC_IMPL_H */

