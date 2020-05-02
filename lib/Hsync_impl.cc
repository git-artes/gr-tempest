/* -*- c++ -*- */
/* 
 * Copyright 2020 <+YOU OR YOUR COMPANY+>.
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
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <gnuradio/io_signature.h>
#include "Hsync_impl.h"
#include <volk/volk.h>
#include <math.h>
#include <gnuradio/math.h>


namespace gr {
    namespace tempest {

        Hsync::sptr
            Hsync::make(int Htotal, int delay)
            {
                return gnuradio::get_initial_sptr
                    (new Hsync_impl(Htotal, delay));
            }

        /*
         * The private constructor
         */
        Hsync_impl::Hsync_impl(int Htotal, int delay)
            : gr::block("Hsync",
                    gr::io_signature::make(1, 1, sizeof(gr_complex)),
                    gr::io_signature::make(1, 1, sizeof(gr_complex)))
        {
            set_relative_rate(1);
            d_delay = delay;
            d_Htotal = Htotal;
            d_initial_acquisition = 0;

            //VOLK alignment as recommended by GNU Radio's Manual. It has a similar effect 
            //than set_output_multiple(), thus we will generally get multiples of this value
            //as noutput_items. 
            const int alignment_multiple = volk_get_alignment() / sizeof(gr_complex);
            set_alignment(std::max(1, alignment_multiple));

            //I'll generate complete lines per call to the block
           set_output_multiple(d_Htotal);

            d_corr = new gr_complex[d_Htotal + d_delay];
            if (d_corr == NULL)
                std::cout << "cannot allocate memory: d_corr" << std::endl;
            d_abs_corr = new float[d_Htotal + d_delay];
            if (d_abs_corr == NULL)
                std::cout << "cannot allocate memory: d_abs_corr" << std::endl;
            
           //d_peak_epsilon = 0;
            //peak_detect_init(0.2, 0.25, 30, 0.0005);
            // peak_detect_init(0.8, 0.9, 30, 0.9);
            peak_detect_init(0.5, 0.9);
            
            d_previous_line_start = 0;
            d_previous_peak_epsilon = 0;

        }

        /*
         * Our virtual destructor.
         */
        Hsync_impl::~Hsync_impl()
        {
            delete [] d_corr;
        }

        void 
            Hsync_impl::peak_detect_init(float threshold_factor_rise, float alpha)
            {
                d_avg_alpha = alpha;
                d_threshold_factor_rise = threshold_factor_rise;
                d_avg_max = - (float)INFINITY;
                d_avg_min =   (float)INFINITY;
            }


        int 
            Hsync_impl::peak_detect_process(const float * datain, const int datain_length, int * peak_pos)
            {
#if VOLK_GT_122
                uint16_t peak_index = 0;
                uint32_t d_datain_length = (uint32_t)datain_length;
#else
                unsigned int peak_index = 0;
                int d_datain_length = datain_length;
#endif
                bool success = true;

                volk_32f_index_max_16u(&peak_index, &datain[0], d_datain_length); 

                if (datain_length>=d_Htotal){
                    float min = datain[(peak_index + d_Htotal/2) % d_Htotal];
                    if(d_avg_min==(float)INFINITY){
                        d_avg_min = min;
                    }
                    else 
                    {
                        d_avg_min = d_avg_alpha*min + (1-d_avg_alpha)*d_avg_min;
                    }

                }

                if (d_avg_max==-(float)INFINITY)
                {
                    // I initialize the d_avg with the first value. 
                    d_avg_max = datain[peak_index];
                }
                else if ( datain[ peak_index ] > d_avg_max - d_threshold_factor_rise*(d_avg_max-d_avg_min) ) 
                {
                    d_avg_max = d_avg_alpha * datain[ peak_index ] + (1 - d_avg_alpha) * d_avg_max;
                }
                else
                {
                    // if the peak is not large enough, we declare this a failure
                    success = false; 
                }

                //printf("d_avg_max: %f\n",d_avg_max);
                //printf("d_avg_min: %f\n",d_avg_min);
                //printf("datain[%i]: %f\n",peak_index, datain[peak_index]);
                //printf("sucess: %d\n",success);
                
                //We now check whether peaks are also present at multiples of d_delay. This may 
                //happen with border that by chance are at the same distance as the d_delay. In this
                //case we will ignore the result of this peak_process, as we don't know whether samples 
                //were lost (and the line has actually changed). 
                for(int i=(peak_index % d_delay); i<datain_length; i=i+d_delay){
                    if((datain[ i ] > d_avg_max - d_threshold_factor_rise*(d_avg_max-d_avg_min)) && i!=peak_index  ){
                        // i.e. it could also be a peak according to our criteria, we thus declare this a failure
                        success = false;
                    }
                }


                *peak_pos = peak_index;
                return (success);

            }

        int
            Hsync_impl::max_corr_sync(const gr_complex * in, int lookup_start, int lookup_stop, int * max_corr_pos, float * peak_epsilon)
            {

                assert(lookup_start >= lookup_stop);
                //assert(lookup_stop >= (d_Htotal - 1));

                int size;

                // I calculate the 1-sample correlation
                size = lookup_start - lookup_stop;
                //volk_32fc_x2_multiply_conjugate_32fc(&d_corr[0], &in[lookup_start], &in[lookup_start - d_delay], size);
                volk_32fc_x2_multiply_conjugate_32fc(&d_corr[0], &in[lookup_stop+d_delay], &in[lookup_stop], size);

                // I calculate its magnitude
                size = lookup_start - lookup_stop;
                volk_32fc_magnitude_32f(&d_abs_corr[0], &d_corr[0], size);

                int peak = 0;
                bool found_peak = true; 

                // Find peaks of lambda
                // We have found an end of symbol at peak_pos[0] + CP + FFT
                if ((found_peak = peak_detect_process(&d_abs_corr[0], (lookup_start - lookup_stop), &peak)))
                {
                    *max_corr_pos = peak + lookup_stop;

                    // Calculate frequency correction
                    *peak_epsilon = fast_atan2f(d_corr[peak]);
                }
                return (found_peak);

           }

        // Derotates the signal 
        // TODO implement it?
        void 
            Hsync_impl::derotate(const gr_complex * in, gr_complex * out)
            {
                memcpy(out, in, d_Htotal*sizeof(gr_complex));
                //double sensitivity = (double)(-1) / (double)d_fft_length;
                //d_phaseinc = sensitivity * d_peak_epsilon;

                //gr_complex phase_increment = gr_complex(std::cos(d_phaseinc), std::sin(d_phaseinc)); 
                //gr_complex phase_current = gr_complex(std::cos(d_phase), std::sin(d_phase)); 

                //volk_32fc_s32fc_x2_rotator_32fc(&out[0], &in[0], phase_increment, &phase_current, d_fft_length) ; 
                //d_phase = std::arg(phase_current); 
                //d_phase = fmod(d_phase + d_phaseinc*d_cp_length, (float)2*M_PI);
           }

        void
            Hsync_impl::forecast (int noutput_items, gr_vector_int &ninput_items_required)
            {
                int ninputs = ninput_items_required.size ();
                // make sure we receive at least (d_delay + dHtotal + 1)
                for (int i = 0; i < ninputs; i++)
                {
                    ninput_items_required[i] = ( d_delay + 2*d_Htotal + 1) * (noutput_items/d_Htotal +1) ;
                    //printf("division: %i\n",noutput_items/d_Htotal);
                    //printf("noutput_items: %i\n",noutput_items);      
                    //printf("ninput_items_required[%i]: %i\n",i,ninput_items_required[i]);      
                }
            }

        int
            Hsync_impl::general_work (int noutput_items,
                    gr_vector_int &ninput_items,
                    gr_vector_const_void_star &input_items,
                    gr_vector_void_star &output_items)
            {
                const gr_complex *in = (const gr_complex *) input_items[0];
                gr_complex *out = (gr_complex *) output_items[0];

                int low, size;

                d_consumed = 0;
                d_out = 0;

                //printf("is_unaligned():%s\n",is_unaligned() ? "True":"False");
                //
                //////////////////////
                // NOW I'M TESTING THE SIMPEST ALGORITHM
                //////////////////////
                //d_initial_acquisition = false;

                for (int line = 0; line < noutput_items/d_Htotal ; line++) {
                    if (!d_initial_acquisition)
                    {
                        // If we are here it means that we have no idea where the max_corr may be. We thus 
                        // search it thoroughly
                        d_initial_acquisition = max_corr_sync(&in[d_consumed], d_Htotal, 0, &d_line_start, &d_peak_epsilon);
                        d_line_found = d_initial_acquisition; 
                        
                        printf("d_initial_acquisition (no idea): %i\n",d_initial_acquisition);      
                        printf("d_line_start (no idea): %i\n",d_line_start);      
                    }
                    else
                    {
                        //If we are here it means that in the previous iteration we found the line. We
                        //now thus only search near it. 
                        d_line_found = max_corr_sync(&in[d_consumed], d_line_start + 8, std::max(d_line_start - 8, 0), &d_line_start, &d_peak_epsilon);
                        printf("d_line_found (search near): %i\n",d_line_found);      
                        printf("d_line_start (search near): %i\n",d_line_start);      
                        if ( !d_line_found )
                        {
                            // We may have not found the max corr because the smaller search range was too small (rare, but possible, 
                            // in particular when sampling time error are present). We thus re-try with a bigger search range and 
                            // d_line_start. 
                            d_line_found = max_corr_sync(&in[d_consumed], d_Htotal, 0, &d_line_start, &d_peak_epsilon );

                            printf("d_line_found (failed near): %i\n",d_line_found);      
                            printf("d_line_start (failed near): %i\n",d_line_start);      
                        }
                    }

                    if ( d_line_found )
                    {
                        //printf("d_line_start: %i\n",d_line_start);      

                        //low = d_cp_start + d_cp_start_offset - d_fft_length + 1 ;
                        derotate(&in[d_consumed+d_line_start], &out[line*d_Htotal]);

                        d_previous_line_start = d_line_start;
                        d_previous_peak_epsilon = d_peak_epsilon;

                        d_out = d_out + d_Htotal; 

                    }
                    else
                    {
                        d_initial_acquisition = false;

                        // as we could not establish the line begin, I'll use the value estimated on the 
                        // previous iteration
                        derotate(&in[d_consumed+d_previous_line_start], &out[line*d_Htotal]);
                        d_out = d_out + d_Htotal; 

                        //// Restart with a half number so that we'll not endup with the same situation
                        //// This will prevent peak_detect to not detect anything
                        //d_consumed += (d_Htotal)/2;
                        //// Tell runtime system how many output items we produced.
                        
                        d_consumed += d_Htotal;
                        consume_each(d_consumed);
                        return (d_out);
                    }

                    d_consumed += d_Htotal;
                }

                // Tell runtime system how many input items we consumed on
                // each input stream.
                consume_each(d_consumed);

                // Tell runtime system how many output items we produced.
                return (d_out);
            }


    } /* namespace tempest */
} /* namespace gr */

