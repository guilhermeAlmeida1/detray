# Detray library, part of the ACTS project (R&D line)
#
# (c) 2022-2023 CERN for the benefit of the ACTS project
#
# Mozilla Public License Version 2.0

# Set up the core tests.
detray_add_test( io
   "io_json_payload.cpp"
   LINK_LIBRARIES GTest::gtest_main detray::core_array detray::io_array )
detray_add_test( io_writer
   "io_json_detector_writer.cpp"
   LINK_LIBRARIES GTest::gtest_main vecmem::core detray::core_array detray::io_array detray::utils_array )
detray_add_test( io_reader
    "io_json_detector_reader.cpp"
    LINK_LIBRARIES GTest::gtest_main vecmem::core detray::core_array
    detray::io_array detray::test detray_tests_common detray::utils_array)
