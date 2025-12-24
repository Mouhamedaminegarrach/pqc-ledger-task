cmake_minimum_required(VERSION 3.15)

find_package(benchmark REQUIRED)

add_executable(pqc-ledger-bench verify.cpp)

target_link_libraries(pqc-ledger-bench
    PRIVATE
        pqc_ledger
        benchmark::benchmark
        benchmark::benchmark_main
)

