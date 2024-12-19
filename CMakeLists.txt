cmake_minimum_required(VERSION 3.27)
project(parallel_fluid)

set(CMAKE_CXX_STANDARD 20)

add_executable(multithread
        multi-thread.cpp
        thread-pool.h)

add_executable(monothread
        mono-thread.cpp)

add_executable(original
        original.cpp)

#target_compile_options(original PRIVATE -O3)
add_executable(thread-pool-test test-thread-pool.cpp)
