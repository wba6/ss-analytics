cmake_minimum_required(VERSION 3.30)
project(ss_analytics)

set(CMAKE_CXX_STANDARD 17)

# Force building performance_analyzer as a library
option(BUILD_AS_LIBRARY "Build performance_analyzer as a static library" ON)
add_compile_definitions(PROFILING_ENABLED)

add_subdirectory(./vendor/performance-analyzer)


add_executable(ss_analytics main.cpp)
target_link_libraries(ss_analytics PRIVATE performance_analyzer)
