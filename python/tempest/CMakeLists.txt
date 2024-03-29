# Copyright 2011 Free Software Foundation, Inc.
#
# This file was generated by gr_modtool, a tool from the GNU Radio framework
# This file is a part of gr-tempest
#
# SPDX-License-Identifier: GPL-3.0-or-later
#

########################################################################
# Include python install macros
########################################################################
include(GrPython)
if(NOT PYTHONINTERP_FOUND)
    return()
endif()

add_subdirectory(bindings)

########################################################################
# Install python sources
########################################################################
GR_PYTHON_INSTALL(
    FILES
    __init__.py
    tempest_msgbtn.py
    message_to_var.py
    image_source.py DESTINATION ${GR_PYTHON_DIR}/gnuradio/tempest
)

########################################################################
# Handle the unit tests
########################################################################
include(GrTest)

set(GR_TEST_TARGET_DEPS gnuradio-tempest)

# Create a package directory that tests can import. It includes everything
# from `python/`.
add_custom_target(
  copy_module_for_tests ALL
  COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_SOURCE_DIR}
          ${CMAKE_BINARY_DIR}/test_modules/gnuradio/tempest/
)
GR_ADD_TEST(qa_fft_peak_fine_sampling_sync ${PYTHON_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/qa_fft_peak_fine_sampling_sync.py)
GR_ADD_TEST(qa_fine_sampling_synchronization ${PYTHON_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/qa_fine_sampling_synchronization.py)
GR_ADD_TEST(qa_frame_drop ${PYTHON_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/qa_frame_drop.py)
GR_ADD_TEST(qa_framing ${PYTHON_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/qa_framing.py)
GR_ADD_TEST(qa_infer_screen_resolution ${PYTHON_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/qa_infer_screen_resolution.py)
GR_ADD_TEST(qa_normalize_flow ${PYTHON_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/qa_normalize_flow.py)
GR_ADD_TEST(qa_sampling_synchronization ${PYTHON_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/qa_sampling_synchronization.py)
GR_ADD_TEST(qa_ssamp_correction ${PYTHON_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/qa_ssamp_correction.py)
GR_ADD_TEST(qa_sync_detector ${PYTHON_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/qa_sync_detector.py)
GR_ADD_TEST(qa_tempest_msgbtn ${PYTHON_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/qa_tempest_msgbtn.py)
GR_ADD_TEST(qa_message_to_var ${PYTHON_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/qa_message_to_var.py)
GR_ADD_TEST(qa_image_source ${PYTHON_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/qa_image_source.py)
