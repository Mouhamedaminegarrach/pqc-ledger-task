cmake_minimum_required(VERSION 3.15)
project(pqc-ledger-task VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Build options
option(BUILD_TESTS "Build tests" ON)
option(BUILD_BENCHMARKS "Build benchmarks" ON)
option(BUILD_CLI "Build CLI tool" ON)

# Output directories
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# Include directories
include_directories(${CMAKE_SOURCE_DIR}/include)

# Find dependencies
find_package(OpenSSL QUIET)
if(NOT OpenSSL_FOUND)
    message(WARNING "OpenSSL not found. Some features may not work.")
endif()

# Find liboqs
# Try to find liboqs as a package first
find_package(liboqs QUIET)

# If not found, try to find it via pkg-config or common paths
if(NOT liboqs_FOUND)
    # Try common installation paths
    find_path(LIBOQS_INCLUDE_DIR
        NAMES oqs/oqs.h
        PATHS
            /usr/local/include
            /usr/include
            ${CMAKE_SOURCE_DIR}/third_party/liboqs/build/include
            ${CMAKE_SOURCE_DIR}/third_party/liboqs/include
    )
    
    find_library(LIBOQS_LIBRARY
        NAMES oqs liboqs
        PATHS
            /usr/local/lib
            /usr/lib
            ${CMAKE_SOURCE_DIR}/third_party/liboqs/build/lib/Release
            ${CMAKE_SOURCE_DIR}/third_party/liboqs/build/lib
            ${CMAKE_SOURCE_DIR}/third_party/liboqs/.libs
    )
    
    if(LIBOQS_INCLUDE_DIR AND LIBOQS_LIBRARY)
        message(STATUS "Found liboqs: ${LIBOQS_LIBRARY}")
        set(liboqs_FOUND TRUE)
    endif()
endif()

# If still not found, provide instructions
if(NOT liboqs_FOUND)
    message(WARNING "liboqs not found. Please install liboqs or set LIBOQS_INCLUDE_DIR and LIBOQS_LIBRARY.")
    message(STATUS "To build liboqs:")
    message(STATUS "  git clone https://github.com/open-quantum-safe/liboqs.git")
    message(STATUS "  cd liboqs && mkdir build && cd build")
    message(STATUS "  cmake .. && make")
    message(STATUS "Then set: -DLIBOQS_INCLUDE_DIR=/path/to/liboqs/build/include")
    message(STATUS "         -DLIBOQS_LIBRARY=/path/to/liboqs/build/lib/liboqs.a")
endif()

# Library sources
set(LIB_SOURCES
    src/codec/encode.cpp
    src/codec/decode.cpp
    src/crypto/hash.cpp
    src/crypto/pq.cpp
    src/crypto/address.cpp
    src/crypto/classical.cpp
    src/tx/signing.cpp
    src/tx/validation.cpp
)

# Add OpenSSL define if found
if(OpenSSL_FOUND)
    add_definitions(-DHAVE_OPENSSL)
endif()

# Library headers
set(LIB_HEADERS
    include/pqc_ledger/pqc_ledger.hpp
    include/pqc_ledger/error.hpp
    include/pqc_ledger/types.hpp
    include/pqc_ledger/codec/encode.hpp
    include/pqc_ledger/codec/decode.hpp
    include/pqc_ledger/crypto/hash.hpp
    include/pqc_ledger/crypto/pq.hpp
    include/pqc_ledger/crypto/address.hpp
    include/pqc_ledger/crypto/classical.hpp
    include/pqc_ledger/tx/signing.hpp
    include/pqc_ledger/tx/validation.hpp
)

# Create library
add_library(pqc_ledger STATIC ${LIB_SOURCES} ${LIB_HEADERS})

if(OpenSSL_FOUND)
    target_link_libraries(pqc_ledger
        PUBLIC
            OpenSSL::SSL
            OpenSSL::Crypto
    )
endif()

# Link liboqs
if(liboqs_FOUND)
    if(TARGET liboqs::liboqs)
        # If found via find_package with proper targets
        target_link_libraries(pqc_ledger PRIVATE liboqs::liboqs)
    elseif(LIBOQS_INCLUDE_DIR AND LIBOQS_LIBRARY)
        # If found via manual paths
        target_include_directories(pqc_ledger PRIVATE "${LIBOQS_INCLUDE_DIR}")
        target_link_libraries(pqc_ledger PRIVATE "${LIBOQS_LIBRARY}")
    endif()
    message(STATUS "liboqs linked successfully")
else()
    message(WARNING "liboqs not linked - PQ functions will not work")
endif()

target_include_directories(pqc_ledger
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

# CLI executable
if(BUILD_CLI)
    add_executable(pqc-ledger-cli src/cli/main.cpp)
    target_link_libraries(pqc-ledger-cli PRIVATE pqc_ledger)
endif()

# Tests
if(BUILD_TESTS)
    enable_testing()
    find_package(GTest QUIET)
    
    if(NOT GTest_FOUND)
        # Use FetchContent to download GTest if not found
        include(FetchContent)
        FetchContent_Declare(
            googletest
            GIT_REPOSITORY https://github.com/google/googletest.git
            GIT_TAG v1.14.0
        )
        # For Windows: Prevent overriding the parent project's compiler/linker settings
        set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
        FetchContent_MakeAvailable(googletest)
        message(STATUS "GTest downloaded via FetchContent")
    else()
        message(STATUS "GTest found via find_package")
    endif()
    
    add_subdirectory(tests)
endif()

# Benchmarks
if(BUILD_BENCHMARKS)
    find_package(benchmark QUIET)
    
    if(benchmark_FOUND)
        add_subdirectory(benches)
    else()
        message(WARNING "Google Benchmark not found. Benchmarks will not be built.")
    endif()
endif()

# Installation
install(TARGETS pqc_ledger
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
    RUNTIME DESTINATION bin
)

install(DIRECTORY include/pqc_ledger
    DESTINATION include
    FILES_MATCHING PATTERN "*.hpp"
)

