#include "performance-analyzer/Profiler.h"

#ifdef __APPLE__
#include <OpenCL/cl.h>
#else
#include <CL/cl.h>
#endif

#include <climits>
#include <ctime>
#include <iostream>
#include <sys/types.h>
#define CL_HPP_ENABLE_EXCEPTIONS
#include <string>
#define CL_HPP_TARGET_OPENCL_VERSION 120 
#define CL_HPP_MINIMUM_OPENCL_VERSION 120
#include "CL/opencl.hpp"



void print_cl_time(cl::Event &event); 
//This function can throw exceptions
int clSearch(const std::string& str, const std::string& substr) {
    PROFILE_FUNCTION();
    int hostResult = INT_MAX;

    Timer setupTimer("Setup Context and Queue");
    // Create context (first available device)
    cl::Context context(CL_DEVICE_TYPE_DEFAULT);

    // Create a command queue
    cl::CommandQueue queue(context,  CL_QUEUE_PROFILING_ENABLE);
    setupTimer.stop();

    //Each work-item checks if 'substr' occurs at position `i` of 'str'.
    std::string kernelSource = R"(
        __kernel void search(__global const char* str, __constant const char* substr,
                             __global int* result, int str_length, int substr_length) {
            int i = get_global_id(0);
            // Only valid starting indices
            if (i > str_length - substr_length) return;
            // Compare the substring
            for (int j = 0; j < substr_length; j++) {
                if (str[i + j] != substr[j])
                    return;
            }
            // Match found: update result with the minimal index.
            atomic_min(result, i);
        }
    )";
    // build the program
    Timer buildTimer("Build Program");
    // Build the program.
    cl::Program program(context, kernelSource);
    try {
        program.build();
    } catch (cl::Error& e) {
        // In case of a build error, print the build log.
        auto devices = context.getInfo<CL_CONTEXT_DEVICES>();
        std::cerr << "Build failed for device: "
                  << program.getBuildInfo<CL_PROGRAM_BUILD_LOG>(devices[0])
                  << std::endl;
        throw;
    }
    buildTimer.stop();

    Timer bufferTimer("Create Buffers");
    int numTextElements = str.length();
    int textLen    = numTextElements;
    int patternLen = substr.length();


    // Copy the text and pattern data to device
    cl::Buffer d_text(
        context,
        CL_MEM_READ_ONLY | CL_MEM_USE_HOST_PTR,
        sizeof(cl_char) * numTextElements,
        (void*)str.data()  // textVector is your repacked array of cl_char16 elements
   );


    cl::Buffer d_pattern(
        context,
        CL_MEM_READ_ONLY  | CL_MEM_USE_HOST_PTR,
        sizeof(cl_char) * patternLen,
        (void*)substr.data()
    );

    cl::Buffer d_result(
        context,
        CL_MEM_READ_WRITE | CL_MEM_USE_HOST_PTR,
        sizeof(int),
        &hostResult
    );

    bufferTimer.stop();

    // used for timing kernel
    cl::Event event;

    // run kernel
    {
        PROFILE_SCOPE("Run Kernel");

        cl::Kernel kernel(program, "search");
        kernel.setArg(0, d_text);
        kernel.setArg(1, d_pattern);
        kernel.setArg(2, d_result);
        kernel.setArg(3, textLen);
        kernel.setArg(4, patternLen);
        
        // Enqueue kernel:
        Timer enqueTimer("enqueueNDRangeKernel");
        int error = queue.enqueueNDRangeKernel(
            kernel,
            cl::NullRange,
            cl::NDRange(numTextElements),
            cl::NullRange,
            nullptr,
            &event
        );

        std::cout << "Error Value " << error << std::endl;

        enqueTimer.stop();
        event.wait();
        queue.finish();
    }

    // print cl timing
    print_cl_time(event);

    // Read back the result
    Timer readTimer("Read Result");
    cl::copy(queue, d_result, &hostResult, &hostResult + 1);
    readTimer.stop();
    return hostResult;
}

void print_cl_time(cl::Event &event) {
    
    // returns the time passed in milliseconds
    auto calcTime = [](cl_ulong &time_start, cl_ulong &time_end) {
        return (time_end - time_start) /1000000.0; 
    };

    // get timing event
    cl_ulong time_start = 0;
    cl_ulong time_end = 0;

    event.getProfilingInfo(CL_PROFILING_COMMAND_QUEUED, &time_start);
    event.getProfilingInfo(CL_PROFILING_COMMAND_SUBMIT, &time_end);
    std::cout << "OpenCL Queued submision time " << calcTime(time_start, time_end) << " milliseconds" << std::endl;  


    event.getProfilingInfo(CL_PROFILING_COMMAND_START, &time_start);
    event.getProfilingInfo(CL_PROFILING_COMMAND_END, &time_end);
    std::cout << "OpenCL exectution time is " << calcTime(time_start, time_end) << " milliseconds" << std::endl;  
}
