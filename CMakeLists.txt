cmake_minimum_required(VERSION 3.18 FATAL_ERROR)

project(ptx_sha256 LANGUAGES CXX)

# Set c++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Optimization flags
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -march=native -DNDEBUG")
set(CMAKE_BUILD_TYPE Release)

# Find CUDA libs and includes to link against
find_package(CUDAToolkit REQUIRED)

# Set CUDA architecture for PTX JIT compilation
# This tells the JIT compiler to optimize for sm_120 (RTX 5070)
set(CMAKE_CUDA_ARCHITECTURES 120)

set(ptx_directory "${CMAKE_CURRENT_SOURCE_DIR}/ptx")
set(cuda_includes ${CUDAToolkit_INCLUDE_DIRS})
set(includes_directory "${CMAKE_CURRENT_SOURCE_DIR}/include")
set(src_directory "${CMAKE_CURRENT_SOURCE_DIR}/src")

# Test executable
add_executable(test_ptx_sha256 
    tests/test_ptx_sha256.cpp
    src/sha256.cpp
)

target_include_directories(test_ptx_sha256
    PRIVATE ${includes_directory}
    PRIVATE ${cuda_includes}
)

target_compile_definitions(test_ptx_sha256 
    PRIVATE PTX_INCLUDE_DIR="${ptx_directory}"
)

target_link_libraries(test_ptx_sha256 
    CUDA::cuda_driver 
    CUDA::cudart_static
)

# Visual studio setup
set_property(GLOBAL PROPERTY USE_FOLDERS ON)
set_property(DIRECTORY PROPERTY VS_STARTUP_PROJECT test_ptx_sha256)
