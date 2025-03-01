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
    cl::CommandQueue queue(context, CL_QUEUE_OUT_OF_ORDER_EXEC_MODE_ENABLE);
    delete setupTimer;

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
    Timer* buildTimer = new Timer("Build Program");
    cl::Program program(context, kernelSource, true);
    program.build();
    delete buildTimer;

    Timer *bufferTimer = new Timer("Create Buffers");
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
        Timer* enqueTimer = new Timer("enqueueNDRangeKernel");
        queue.enqueueNDRangeKernel(
            kernel,
            cl::NullRange,
            cl::NDRange(numTextElements),
            cl::NullRange
        );
        delete enqueTimer;
        queue.finish();
    }

    // Read back the result
    Timer* readTimer = new Timer("Read Result");
    cl::copy(queue, d_result, &hostResult, &hostResult + 1);
    delete readTimer;
    return hostResult;
}
