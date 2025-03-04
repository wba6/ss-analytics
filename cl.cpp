#include "performance-analyzer/Timer.h"
#include <ctime>
#include <iostream>
#include <sys/types.h>
#define CL_HPP_TARGET_OPENCL_VERSION 300
#define CL_HPP_ENABLE_EXCEPTIONS
#include <string>
#include <CL/opencl.hpp>
#include "performance-analyzer/Profiler.h"


void print_cl_time(cl::Event &event); 
//This function can throw exceptions
int clSearch(const std::string& str, const std::string& substr) {
    PROFILE_FUNCTION();
    int hostResult = -1;

    Timer setupTimer("Setup Context and Queue");
    // Create context (first available device)
    cl::Context context(CL_DEVICE_TYPE_DEFAULT);

    // Create a command queue
    cl::CommandQueue queue(context, CL_QUEUE_OUT_OF_ORDER_EXEC_MODE_ENABLE | CL_QUEUE_PROFILING_ENABLE);
    setupTimer.stop();

    //Each work-item checks if 'substr' occurs at position `i` of 'str'.
    std::string kernelSource = R"(
   __kernel void searchNaive(
    __global const char16 *text,
    __global const char16 *pattern,
    const int textLen,     // number of char16 elements in text
    const int patternLen,  // number of char16 elements in pattern
    __global int *result
)
{
    int i = get_global_id(0);

    // Check if there's enough room for the pattern starting at position i
    if (i + patternLen <= textLen) {
        // Compare each char16 element of the pattern
        for (int j = 0; j < patternLen; j++) {
            // Use all() to check that every component of the vector matches.
            if (!all(text[i + j] == pattern[j])) {
                return;  // Mismatch: exit this work-item early.
            }
        }
        // If we reach here, the pattern matched at position i.
        // Write the match index to the result (overwrites previous matches).
        *result = i;
    }
}    )";

    // build the program
    Timer buildTimer("Build Program");
    cl::Program program(context, kernelSource, true);
    program.build();
    buildTimer.stop();

    Timer bufferTimer("Create Buffers");
    //int textLen    = static_cast<int>(str.length());
    //int patternLen = static_cast<int>(substr.length());
    int numTextElements = (str.length() + 15) / 16;
    int textLen    = numTextElements;
    int patternLen = (substr.length() + 15) / 16;


    // Copy the text and pattern data to device

    cl::Buffer d_text(
        context,
        CL_MEM_READ_ONLY | CL_MEM_USE_HOST_PTR,
        sizeof(cl_char16) * numTextElements,
        (void*)str.data()  // textVector is your repacked array of cl_char16 elements
   );


    cl::Buffer d_pattern(
        context,
        CL_MEM_READ_ONLY  | CL_MEM_USE_HOST_PTR,
        sizeof(cl_char16) * patternLen,
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
    cl_event event_obj;
    cl::Event event(event_obj);

    // run kernel
    {
        PROFILE_SCOPE("Run Kernel");

        cl::Kernel kernel(program, "searchNaive");
        kernel.setArg(0, d_text);
        kernel.setArg(1, d_pattern);
        kernel.setArg(2, textLen);
        kernel.setArg(3, patternLen);
        kernel.setArg(4, d_result);
        
        // Enqueue kernel:
        Timer enqueTimer("enqueueNDRangeKernel");
        queue.enqueueNDRangeKernel(
            kernel,
            cl::NullRange,
            cl::NDRange(numTextElements),
            cl::NullRange,
            nullptr,
            &event
        );

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


    event.getProfilingInfo(CL_PROFILING_COMMAND_END, &time_start);
    event.getProfilingInfo(CL_PROFILING_COMMAND_COMPLETE, &time_end);
    std::cout << "OpenCL execution to completion time " << calcTime(time_start, time_end) << " milliseconds" << std::endl;  

}
