# Copyright 2011,2012,2016 Free Software Foundation, Inc.
#
# This file is part of GNU Radio
#
# GNU Radio is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 3, or (at your option)
# any later version.
#
# GNU Radio is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with GNU Radio; see the file COPYING.  If not, write to
# the Free Software Foundation, Inc., 51 Franklin Street,
# Boston, MA 02110-1301, USA.

########################################################################
# Setup library
########################################################################
include(GrPlatform) #define LIB_SUFFIX

include_directories(${Boost_INCLUDE_DIR})
link_directories(${Boost_LIBRARY_DIRS})

list(APPEND tempest_sources
    Hsync_impl.cc
)

set(tempest_sources "${tempest_sources}" PARENT_SCOPE)
if(NOT tempest_sources)
	MESSAGE(STATUS "No C++ sources... skipping lib/")
	return()
endif(NOT tempest_sources)

add_library(gnuradio-tempest SHARED ${tempest_sources})
target_link_libraries(gnuradio-tempest ${Boost_LIBRARIES} ${GNURADIO_ALL_LIBRARIES})
set_target_properties(gnuradio-tempest PROPERTIES DEFINE_SYMBOL "gnuradio_tempest_EXPORTS")

if(APPLE)
    set_target_properties(gnuradio-tempest PROPERTIES
        INSTALL_NAME_DIR "${CMAKE_INSTALL_PREFIX}/lib"
    )
endif(APPLE)

########################################################################
# Install built library files
########################################################################
include(GrMiscUtils)
GR_LIBRARY_FOO(gnuradio-tempest RUNTIME_COMPONENT "tempest_runtime" DEVEL_COMPONENT "tempest_devel")

#######################################################################
## Workaround for issues with the change of VOLK API
## See https://github.com/gnuradio/volk/issues/84
## and https://github.com/git-artes/gr-isdbt/issues/18
#######################################################################

if(NOT VOLK_VERSION)
    set(VOLK_VERSION "1.2.2")
endif(NOT VOLK_VERSION)

if(VOLK_VERSION VERSION_EQUAL 1.2.2)
    find_file(VOLK_GREATER_THAN_122
        NAMES volk/volk_32f_index_max_32u.h
        HINTS ${CMAKE_INSTALL_PREFIX}/include
        PATHS /usr/local/include
        /usr/include
        ${CMAKE_INSTALL_PREFIX}/include
        )
    if(VOLK_GREATER_THAN_122)
        set(VOLK_VERSION "1.2.3+")
    endif(VOLK_GREATER_THAN_122)
endif(VOLK_VERSION VERSION_EQUAL 1.2.2)

if(VOLK_VERSION VERSION_GREATER 1.2.2)
    add_definitions(-DVOLK_GT_122=1)
endif(VOLK_VERSION VERSION_GREATER 1.2.2)



########################################################################
# Build and register unit test
########################################################################
include(GrTest)

include_directories(${CPPUNIT_INCLUDE_DIRS})

list(APPEND test_tempest_sources
    ${CMAKE_CURRENT_SOURCE_DIR}/test_tempest.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/qa_tempest.cc
)

add_executable(test-tempest ${test_tempest_sources})

target_link_libraries(
  test-tempest
  ${GNURADIO_RUNTIME_LIBRARIES}
  ${Boost_LIBRARIES}
  ${CPPUNIT_LIBRARIES}
  gnuradio-tempest
)

GR_ADD_TEST(test_tempest test-tempest)

########################################################################
# Print summary
########################################################################
message(STATUS "Using install prefix: ${CMAKE_INSTALL_PREFIX}")
message(STATUS "Building for version: ${VERSION} / ${LIBVER}")

