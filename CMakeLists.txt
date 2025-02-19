cmake_minimum_required(VERSION 3.30)
project(ss_analytics LANGUAGES CXX ASM_NASM)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Force building performance_analyzer as a library
option(BUILD_AS_LIBRARY "Build performance_analyzer as a static library" ON)
add_compile_definitions(PROFILING_ENABLED)

include_directories(./vendor)
add_subdirectory(./vendor/performance-analyzer)

add_executable(ss_analytics main.cpp containsASM.asm cl.cpp implimentedFunctions.cpp standardFunctions.cpp)

# Apply AVX2 only for C++ source files
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(ss_analytics PRIVATE $<$<COMPILE_LANGUAGE:CXX>:-mavx2>)
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    target_compile_options(ss_analytics PRIVATE $<$<COMPILE_LANGUAGE:CXX>:/arch:AVX2>)
endif()

find_package(OpenCL REQUIRED)
target_compile_features(ss_analytics PRIVATE cxx_auto_type)
target_link_libraries(ss_analytics PRIVATE performance_analyzer OpenCL::OpenCL)
