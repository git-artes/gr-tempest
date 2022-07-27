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


#ifndef INCLUDED_TEMPEST_FRAMING_H
#define INCLUDED_TEMPEST_FRAMING_H

#include <gnuradio/tempest/api.h>
#include <gnuradio/block.h>

namespace gr {
  namespace tempest {

    /*!
     * \brief <+description of block+>
     * \ingroup tempest
     *
     */
    class TEMPEST_API framing : virtual public gr::block
    {
     public:
      typedef std::shared_ptr<framing> sptr;

      /*!
       * \brief Return a shared_ptr to a new instance of tempest::framing.
       *
       * To avoid accidental use of raw pointers, tempest::framing's
       * constructor is in a private implementation
       * class. tempest::framing::make is the public interface for
       * creating new instances.
       */
      static sptr make(int Htotal, int Vtotal, int Hdisplay, int Vdisplay);
      
      virtual void set_Htotal_and_Vtotal(int Htotal, int Vtotal) = 0;
    };

  } // namespace tempest
} // namespace gr

#endif /* INCLUDED_TEMPEST_FRAMING_H */

