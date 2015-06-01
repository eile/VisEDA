# Copyright (c) HBP 2014-2015 Daniel.Nachbaur@epfl.ch
#                             Stefan.Eilemann@epfl.ch
# Change this number when adding tests to force a CMake run: 3

set(TEST_LIBRARIES ${Boost_UNIT_TEST_FRAMEWORK_LIBRARY} zeq zeqHBP zerobuf)
flatbuffers_generate_c_headers(FBS_NEW_EVENT newEvent.fbs)
set_source_files_properties(
  newEvent.cpp PROPERTIES OBJECT_DEPENDS ${FBS_NEW_EVENT_OUTPUTS})

flatbuffers_generate_c_headers(FBS_BINARY_EVENT binary.fbs)
set_source_files_properties(
  serialization.cpp PROPERTIES OBJECT_DEPENDS ${FBS_BINARY_EVENT_OUTPUTS})

include(CommonCTest)
