cmake_minimum_required(VERSION 3.30)
project(ss_analytics LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Force building performance_analyzer as a library
option(BUILD_AS_LIBRARY "Build performance_analyzer as a static library" ON)
add_compile_definitions(PROFILING_ENABLED)

add_subdirectory(./vendor/performance-analyzer)

add_executable(ss_analytics 
    main.cpp
    searchFunctions/cl.cpp
    searchFunctions/implementedFunctions.cpp
    searchFunctions/standardFunctions.cpp
    benchMarker.cpp
)


if (MSVC)
        target_compile_options(ss_analytics PRIVATE
                $<$<CONFIG:DEBUG>:/Zi>
                $<$<CONFIG:RELEASE>:/O2>
        )
else()
        target_compile_options(ss_analytics PRIVATE
                $<$<CONFIG:DEBUG>:-g>
                $<$<CONFIG:RELEASE>:-O3>
        )
endif()

find_package(OpenCL REQUIRED)
target_compile_features(ss_analytics PRIVATE cxx_auto_type)
target_link_libraries(ss_analytics PRIVATE performance_analyzer OpenCL::OpenCL) 
target_include_directories( ss_analytics PUBLIC 
    ${PROJECT_SOURCE_DIR}/vendor
    ${PROJECT_SOURCE_DIR}/vendor/performance_analyzer
    ${PROJECT_SOURCE_DIR}/vendor/CL/include
)
