cmake_minimum_required(VERSION 3.15)

# GTest should be available from parent CMakeLists.txt (via find_package or FetchContent)
if(NOT TARGET GTest::gtest AND NOT TARGET gtest)
    message(FATAL_ERROR "GTest not available. Please ensure BUILD_TESTS is ON and GTest is found.")
endif()

# Test executables
add_executable(test_integration_roundtrip integration_roundtrip.cpp)
add_executable(test_mutation mutation.cpp)
add_executable(test_replay replay.cpp)
add_executable(test_codec_encoding codec_encoding.cpp)

# Helper function to link GTest (handles both find_package and FetchContent)
function(link_gtest target)
    if(TARGET GTest::gtest)
        target_link_libraries(${target} PRIVATE GTest::gtest GTest::gtest_main)
    elseif(TARGET gtest)
        target_link_libraries(${target} PRIVATE gtest gtest_main)
    else()
        message(FATAL_ERROR "GTest target not found")
    endif()
endfunction()

# Link tests
target_link_libraries(test_integration_roundtrip PRIVATE pqc_ledger)
link_gtest(test_integration_roundtrip)

target_link_libraries(test_mutation PRIVATE pqc_ledger)
link_gtest(test_mutation)

target_link_libraries(test_replay PRIVATE pqc_ledger)
link_gtest(test_replay)

target_link_libraries(test_codec_encoding PRIVATE pqc_ledger)
link_gtest(test_codec_encoding)

# Add tests to CTest
add_test(NAME IntegrationRoundtrip COMMAND test_integration_roundtrip)
add_test(NAME Mutation COMMAND test_mutation)
add_test(NAME Replay COMMAND test_replay)
add_test(NAME CodecEncoding COMMAND test_codec_encoding)

