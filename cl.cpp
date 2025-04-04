#include "performance-analyzer/performance-analyzer.hpp"
#include <vector>

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



void record_cl_time(cl::Event &event); 
//This function can throw exceptions
std::vector<int> clSearch(const std::string& str, const std::string& substr) {
    PROFILE_FUNCTION();
    std::vector<int> hostResults(100, -1);
    int resultCount {0};

    Timer setupTimer("Setup Context and Queue");

    // Create context (first available device)
    cl::Context context(CL_DEVICE_TYPE_DEFAULT);

    // Create a command queue
    cl::CommandQueue queue(context,  CL_QUEUE_PROFILING_ENABLE);
    setupTimer.stop();

    //Each work-item checks if 'substr' occurs at position `i` of 'str'.
    std::string kernelSource = R"(
    __kernel void searchAllLimited(__global const char* str,
                                   __constant const char* substr,
                                   __global int* outMatches,
                                   __global int* matchCount,
                                   int strLen,
                                   int subLen)
    {
        int i = get_global_id(0);

        if (i <= strLen - subLen) {
            // Compare substring at position i
            for (int j = 0; j < subLen; ++j) {
                if (str[i + j] != substr[j]) {
                    return; // Not a match
                }
            }
            // It's a match: use an atomic to reserve a slot, if any remain
            int slot = atomic_inc(matchCount);
            // If we still have space in our array, record the position
            if (slot < 100) {
                outMatches[slot] = i;
            }
        }
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
        CL_MEM_WRITE_ONLY,
        sizeof(int) * 100
    );

    cl::Buffer d_matchCount(
        context,
        CL_MEM_READ_WRITE | CL_MEM_USE_HOST_PTR,
        sizeof(int),
        &resultCount
    );


    bufferTimer.stop();

    // used for timing kernel
    cl::Event event;

    // run kernel
    {
        PROFILE_SCOPE("Run Kernel");

        cl::Kernel kernel(program, "searchAllLimited");
        kernel.setArg(0, d_text);
        kernel.setArg(1, d_pattern);
        kernel.setArg(2, d_result);
        kernel.setArg(3, d_matchCount);
        kernel.setArg(4, textLen);
        kernel.setArg(5, patternLen);
        
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
        enqueTimer.stop();

        std::cout << "CL Error Value " << error << std::endl;

        event.wait();
        queue.finish();

        // record cl timing
        record_cl_time(event);

    }

    // Read back the result
    Timer readTimer("Read Result");
    // Get the final value of matchCount
    queue.enqueueReadBuffer(d_matchCount, CL_TRUE, 0, sizeof(int), &resultCount);

    // Read the first min(matchCountHost, 100) entries
    size_t numMatches = std::min((size_t)resultCount, (size_t)100);
    queue.enqueueReadBuffer(d_result, CL_TRUE, 0,
                            sizeof(int)*numMatches,
                            hostResults.data());

    readTimer.stop();
    hostResults.resize(numMatches);
    return std::move(hostResults);
}

void record_cl_time(cl::Event &event) {
    
    // returns the time passed in microseconds 
    auto calcTime = [](cl_ulong &time_start, cl_ulong &time_end) {
        return (time_end - time_start) /1000.0f; 
    };

    // get timing event
    cl_ulong time_start = 0;
    cl_ulong time_end = 0;

    event.getProfilingInfo(CL_PROFILING_COMMAND_QUEUED, &time_start);
    event.getProfilingInfo(CL_PROFILING_COMMAND_SUBMIT, &time_end);
    PROFILE_CUSTOM_TIME("GPU Queue", calcTime(time_start, time_end));
    

    event.getProfilingInfo(CL_PROFILING_COMMAND_START, &time_start);
    event.getProfilingInfo(CL_PROFILING_COMMAND_END, &time_end);
    PROFILE_CUSTOM_TIME("GPU Exec", calcTime(time_start, time_end));
}
