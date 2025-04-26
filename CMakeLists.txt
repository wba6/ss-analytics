cmake_minimum_required(VERSION 3.30)
project(ss_analytics LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Force building performance_analyzer as a library
option(BUILD_AS_LIBRARY "Build performance_analyzer as a static library" ON)
add_compile_definitions(PROFILING_ENABLED)

add_subdirectory(./vendor/performance-analyzer)

set(KOMPUTE_OPT_BUILD_SHADERS        OFF CACHE BOOL "" FORCE)
set(KOMPUTE_OPT_DISABLE_VULKAN_VERSION_CHECK  ON CACHE BOOL "" FORCE)
set(KOMPUTE_OPT_USE_BUILT_IN_FMT     ON CACHE BOOL "" FORCE)
set(KOMPUTE_OPT_USE_SPDLOG           OFF CACHE BOOL "" FORCE)
set(KOMPUTE_OPT_USE_BUILT_IN_GOOGLE_TEST  OFF CACHE BOOL "" FORCE)
set(KOMPUTE_OPT_USE_BUILT_IN_PYBIND11     OFF CACHE BOOL "" FORCE)
set(KOMPUTE_OPT_USE_BUILT_IN_VULKAN_HEADER ON CACHE BOOL "" FORCE)
add_subdirectory(./vendor/kompute)

add_executable(ss_analytics 
    main.cpp
    searchFunctions/cl.cpp
    searchFunctions/implementedFunctions.cpp
    searchFunctions/standardFunctions.cpp
    benchMarker.cpp
    searchFunctions/vkSearch.cpp
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
find_package(Vulkan REQUIRED)
find_package(fmt REQUIRED)
target_compile_features(ss_analytics PRIVATE cxx_auto_type)
target_link_libraries(ss_analytics PRIVATE performance_analyzer OpenCL::OpenCL ${Vulkan_LIBRARIES} kompute::kompute fmt::fmt) 
target_include_directories( ss_analytics PUBLIC 
    ${PROJECT_SOURCE_DIR}/vendor
    ${PROJECT_SOURCE_DIR}/vendor/performance_analyzer
    ${PROJECT_SOURCE_DIR}/vendor/CL/include
    ${Vulkan_INCLUDE_DIR}
    ${CMAKE_SOURCE_DIR}/vendor/kompute/src/include
    ${CMAKE_BINARY_DIR}/vendor/kompute/src/shaders/glsl
)

add_custom_target(run
    DEPENDS ss_analytics
    COMMAND $<TARGET_FILE:ss_analytics> 
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    COMMENT "Running bench mark"
    VERBATIM
)

