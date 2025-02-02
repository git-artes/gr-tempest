/*
 * Copyright 2025 Free Software Foundation, Inc.
 *
 * This file is part of GNU Radio
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 */

/***********************************************************************************/
/* This file is automatically generated using bindtool and can be manually
 * edited  */
/* The following lines can be configured to regenerate this file during cmake */
/* If manual edits are made, the following tags should be modified accordingly.
 */
/* BINDTOOL_GEN_AUTOMATIC(0) */
/* BINDTOOL_USE_PYGCCXML(0) */
/* BINDTOOL_HEADER_FILE(normalize_flow.h) */
/* BINDTOOL_HEADER_FILE_HASH(1a6ec1adf209d7f78e59e092cfccd7a9) */
/***********************************************************************************/

#include <pybind11/complex.h>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

namespace py = pybind11;

#include <gnuradio/tempest/normalize_flow.h>
// pydoc.h is automatically generated in the build directory
#include <normalize_flow_pydoc.h>

void bind_normalize_flow(py::module &m) {

  using normalize_flow = ::gr::tempest::normalize_flow;

  py::class_<normalize_flow, gr::sync_block, gr::block, gr::basic_block,
             std::shared_ptr<normalize_flow>>(m, "normalize_flow",
                                              D(normalize_flow))

      .def(py::init(&normalize_flow::make), py::arg("min"), py::arg("max"),
           py::arg("window"), py::arg("alpha_avg"), py::arg("update_proba"),
           D(normalize_flow, make))

      .def("set_min_max", &normalize_flow::set_min_max, py::arg("min"),
           py::arg("max"), D(normalize_flow, set_min_max))

      ;
}
