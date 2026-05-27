cmake_minimum_required(VERSION 3.16)

project(challenge3 LANGUAGES CXX)

# Use C++23
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Default to Release if not specified
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

# Compiler warnings
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
  add_compile_options(-Wall -Wextra -pedantic)
endif()

# Optimizations
if(CMAKE_BUILD_TYPE STREQUAL "Release")
  if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    add_compile_options(-O3)
  endif()
endif()

# Source files
add_executable(main
  src/main.cpp
)

# Include directories
target_include_directories(main PRIVATE
  ${CMAKE_SOURCE_DIR}/include
  $ENV{HOME}/libraries/eigen
  $ENV{HOME}/libraries/benchmark/include
)

# Compiled Binaries (-L flags)
target_link_directories(main PRIVATE
  $ENV{HOME}/libraries/benchmark/build/src
)

# -lpthread for linking threads
find_package(Threads REQUIRED)

# Link Libraries (-l flags)
target_link_libraries(main PRIVATE
    benchmark
    Threads::Threads
)

# Put executable in bin/
set_target_properties(main PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin
)



add_executable(benchmarks test/benchmark.cpp)
target_include_directories(benchmarks PRIVATE ${CMAKE_SOURCE_DIR}/include $ENV{HOME}/libraries/benchmark/include)
target_link_directories(benchmarks PRIVATE $ENV{HOME}/libraries/benchmark/build/src)
target_link_libraries(benchmarks PRIVATE benchmark Threads::Threads)
set_target_properties(benchmarks PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)