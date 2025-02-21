#include "performance-analyzer/Timer.h"
#include <ctime>
#include <iostream>
#define CL_HPP_TARGET_OPENCL_VERSION 300
#define CL_HPP_ENABLE_EXCEPTIONS
#include <string>
#include <CL/opencl.hpp>
#include "performance-analyzer/Profiler.h"

//This function can throw exceptions
int clSearch(const std::string& str, const std::string& substr) {
    PROFILE_FUNCTION();
    int hostResult = -1;

    Timer* setupTimer = new Timer("Setup Context and Queue");
    // Create context (first available device)
    cl::Context context(CL_DEVICE_TYPE_DEFAULT);

    // Create a command queue
    cl::CommandQueue queue(context);
    delete setupTimer;

    //Each work-item checks if 'substr' occurs at position `i` of 'str'.
    std::string kernelSource = R"(
    __kernel void searchNaive(
        __global const char* text,
        __global const char* pattern,
        const int textLen,
        const int patternLen,
        __global int* result
    )
    {
        int index = get_global_id(0) * 1000000;

        for (int i = index; i <= 1000000; i++) {
        // If there's room for the pattern starting at i:
        if (sizeof(char)*i + patternLen <= textLen)
        {
            // Check each character in the pattern
            for (int j = 0; j < patternLen; j++) {
                if (text[i + j] != pattern[j]) {
                    return;  // Mismatch, so no write to result
                }
            }
            // If we got here, we found a match at position i.
            // Store i in *result. This overwrites previous matches.
            // If you only want the first match, you could avoid overwriting
            // if *result != -1 or use an atomic.
            *result = i;
        }
        }
    }
    )";

    // build the program
    Timer* buildTimer = new Timer("Build Program");
    cl::Program program(context, kernelSource, true);
    program.build();
    delete buildTimer;

    Timer *bufferTimer = new Timer("Create Buffers");
    int textLen    = static_cast<int>(str.size());
    int patternLen = static_cast<int>(substr.size());

    // Copy the text and pattern data to device
    cl::Buffer d_text(
        context,
        CL_MEM_READ_ONLY  | CL_MEM_USE_HOST_PTR,
        sizeof(char) * textLen,
        (void*)str.data()
    );

    cl::Buffer d_pattern(
        context,
        CL_MEM_READ_ONLY  | CL_MEM_USE_HOST_PTR,
        sizeof(char) * patternLen,
        (void*)substr.data()
    );

    cl::Buffer d_result(
        context,
        CL_MEM_READ_WRITE | CL_MEM_USE_HOST_PTR,
        sizeof(int),
        &hostResult
    );
    delete bufferTimer;

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
        queue.enqueueNDRangeKernel(
            kernel,
            cl::NullRange,
            cl::NDRange(str.length()/1000000),
            cl::NullRange
        );
    }

    // Read back the result
    Timer* readTimer = new Timer("Read Result");
    cl::copy(queue, d_result, &hostResult, &hostResult + 1);
    delete readTimer;
    return hostResult;
}
