cmake_minimum_required(VERSION 3.10)

# Project details
project(ScalableThreadManager VERSION 1.0.0 LANGUAGES CXX)

# Demand C++17 at least
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_CXX_EXTENSIONS OFF) # prefer standard over compiler-specific additions

# Create include directories
include_directories(include)

# Let CMake find threads depending on OS
find_package(Threads REQUIRED)

# The primary example executable
add_executable(ThreadManagerExample src/main.cpp)

# Link threading library (like pthreads on linux, does implicit right things on Windows)
target_link_libraries(ThreadManagerExample PRIVATE Threads::Threads)
