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
#include "Vsync_impl.h"
#include <volk/volk.h>

namespace gr {
    namespace tempest {

        Vsync::sptr
            Vsync::make(int Htotal, int Vtotal, int Vblank)
            {
                return gnuradio::get_initial_sptr
                    (new Vsync_impl(Htotal, Vtotal, Vblank));
            }

        /*
         * The private constructor
         */
        Vsync_impl::Vsync_impl(int Htotal, int Vtotal, int Vblank)
            : gr::block("Vsync",
                    gr::io_signature::make(1, 1, sizeof(gr_complex)),
                    gr::io_signature::make(1, 1, sizeof(gr_complex)))
        {
            set_Htotal_and_Vtotal(Htotal, Vtotal);
            set_Vblank(Vblank);
            d_delta = 0;
            d_alpha_energy_avg = 1e-5;

        }

        /*
         * Our virtual destructor.
         */
        Vsync_impl::~Vsync_impl()
        {
        }

        void Vsync_impl::set_Htotal_and_Vtotal(int Htotal, int Vtotal){

            d_Htotal = Htotal;
            d_Vtotal = Vtotal;
            d_lines_processed = 0;

            int old = history()-1;
            
            d_delay = 0;
            set_history(d_delay+1);
            declare_sample_delay(history() - 1);
            d_delta += d_delay - old;
            // I'll output a line at a time
            set_output_multiple(d_Htotal);


            d_energy_per_line = new gr_complex[d_Vtotal];
            if (d_energy_per_line == NULL)
                std::cout << "cannot allocate memory: d_energy_per_line" << std::endl;
            d_energy_avg = new float[d_Vtotal];
            if (d_energy_avg == NULL)
                std::cout << "cannot allocate memory: d_energy_avg" << std::endl;

        }

        void Vsync_impl::set_delay(int delay){
            int old = history()-1;
           
            if (delay != old) { 
                d_delay = delay;
                set_history(d_delay+1);
                declare_sample_delay(history() - 1);
                d_delta += d_delay - old;
            }

        }
        
        int Vsync_impl::get_delay(){
            return d_delay;
        }

        void Vsync_impl::set_Vblank(int Vblank){
            d_Vblank = Vblank;
        }

        void Vsync_impl::forecast(int noutput_items, gr_vector_int& ninput_items_required)
        {
            // make sure all inputs have noutput_items available
            unsigned ninputs = ninput_items_required.size();
            for (unsigned i = 0; i < ninputs; i++)
                ninput_items_required[i] = noutput_items;
        }

        int
            Vsync_impl::general_work(int noutput_items,
                    gr_vector_int &ninput_items,
                    gr_vector_const_void_star &input_items,
                    gr_vector_void_star &output_items)
            {
                const gr_complex *in = (const gr_complex *) input_items[0];
                gr_complex *out = (gr_complex *) output_items[0];


                // update the delay by calculating the energy per-line
                for (int line = 0; line < noutput_items/d_Htotal ; line++) {
                    volk_32fc_x2_conjugate_dot_prod_32fc(&d_energy_per_line[d_lines_processed % d_Vtotal], &in[line*d_Htotal], &in[line*d_Htotal], d_Htotal);
                    d_lines_processed = (d_lines_processed + 1 ) % d_Vtotal;

                }

                for(int line = 0; line < d_Vtotal; line++){
                    for(int j=0; j<d_Vblank; j++){
                        d_energy_avg[line] = (1-d_alpha_energy_avg)*d_energy_avg[line] - d_alpha_energy_avg*std::real(d_energy_per_line[(line+j)%d_Vtotal]);
                    }
                }
#if VOLK_GT_122
                uint16_t peak_index = 0;
                uint32_t d_datain_length = (uint32_t)d_Vtotal;
#else
                unsigned int peak_index = 0;
                int d_datain_length = d_Vtotal;
#endif
                volk_32f_index_max_16u(&peak_index, &d_energy_avg[0], d_datain_length); 

                set_delay(peak_index*d_Htotal);


                ////////////////////////////////
                // Verbatim copy of the delay block in GNU Radio
                ////////////////////////////////

                const gr_complex* iptr;
                gr_complex* optr;
                int cons, ret;

                // No change in delay; just memcpy ins to outs
                d_delta = 0;
                if (d_delta == 0) {
                    for (size_t i = 0; i < input_items.size(); i++) {
                        iptr = (const gr_complex*)input_items[i];
                        optr = (gr_complex*)output_items[i];
                        std::memcpy(optr, iptr, noutput_items * sizeof(gr_complex));
                    }
                    cons = noutput_items;
                    ret = noutput_items;
                }

                consume_each(cons);
                return ret;

                //// Skip over d_delta items on the input
                //else if (d_delta < 0) {
                //    int n_to_copy, n_adj;
                //    int delta = -d_delta;
                //    n_to_copy = std::max(0, noutput_items - delta);
                //    n_adj = std::min(delta, noutput_items);
                //    for (size_t i = 0; i < input_items.size(); i++) {
                //        iptr = (const gr_complex*)input_items[i];
                //        optr = (gr_complex*)output_items[i];
                //        std::memcpy(optr, iptr + delta * sizeof(gr_complex), n_to_copy * sizeof(gr_complex));
                //    }
                //    cons = noutput_items;
                //    ret = n_to_copy;
                //    delta -= n_adj;
                //    d_delta = -delta;
                //}

                //// produce but not consume (inserts zeros)
                //else { // d_delta > 0
                //    int n_from_input, n_padding;
                //    n_from_input = std::max(0, noutput_items - d_delta);
                //    n_padding = std::min(d_delta, noutput_items);
                //    for (size_t i = 0; i < input_items.size(); i++) {
                //        iptr = (const gr_complex*)input_items[i];
                //        optr = (gr_complex*)output_items[i];
                //        std::memset(optr, 0, n_padding * sizeof(gr_complex));
                //        std::memcpy(optr, iptr, n_from_input * sizeof(gr_complex));
                //    }
                //    cons = n_from_input;
                //    ret = noutput_items;
                //    d_delta -= n_padding;
                //}
                //////////////////////////////////
                //// end of Verbatim copy
                //////////////////////////////////

                //return noutput_items;
            }

    } /* namespace tempest */
} /* namespace gr */

