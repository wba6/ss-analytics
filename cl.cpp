#define CL_HPP_TARGET_OPENCL_VERSION 300
#define CL_HPP_ENABLE_EXCEPTIONS
#include <string>
#include <CL/opencl.hpp>
#include "performance-analyzer/Profiler.h"

//This function can throw exceptions
int clSearch(std::string& str){
    PROFILE_FUNCTION();

    // Grab context
    cl::Context context(CL_DEVICE_TYPE_DEFAULT);

    // Create a command queue
    cl::CommandQueue queue(context);

    return 0;
}
