/*
 * Copyright 2022 Free Software Foundation, Inc.
 *
 * This file is part of GNU Radio
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 */

/***********************************************************************************/
/* This file is automatically generated using bindtool and can be manually edited  */
/* The following lines can be configured to regenerate this file during cmake      */
/* If manual edits are made, the following tags should be modified accordingly.    */
/* BINDTOOL_GEN_AUTOMATIC(0)                                                       */
/* BINDTOOL_USE_PYGCCXML(0)                                                        */
/* BINDTOOL_HEADER_FILE(sync_detector.h)                                        */
/* BINDTOOL_HEADER_FILE_HASH(65ca4383a924b017caf558ab02f5a3fe)                     */
/***********************************************************************************/

#include <pybind11/complex.h>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

namespace py = pybind11;

#include <gnuradio/tempest/sync_detector.h>
// pydoc.h is automatically generated in the build directory
#include <sync_detector_pydoc.h>

void bind_sync_detector(py::module& m)
{

    using sync_detector = ::gr::tempest::sync_detector;


    py::class_<sync_detector, gr::block, gr::basic_block, std::shared_ptr<sync_detector>>(
        m, "sync_detector", D(sync_detector))

        .def(py::init(&sync_detector::make),
             py::arg("hscreen"),
             py::arg("vscreen"),
             py::arg("hblanking"),
             py::arg("vblanking"),
             D(sync_detector, make))


        ;
}