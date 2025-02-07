#include <CL/cl.h>
#include <iostream>
#define CL_HPP_TARGET_OPENCL_VERSION 300
#define CL_HPP_ENABLE_EXCEPTIONS
#include <string>
#include <CL/opencl.hpp>
#include "performance-analyzer/Profiler.h"

// Optimized substring search using OpenCL.
// Returns the index of the first occurrence of 'substr' in 'str' (or -1 if not found).
int clSearch(const std::string& str, const std::string& substr) {
    PROFILE_FUNCTION();

    // Quick sanity checks.
    if (substr.empty() || str.size() < substr.size()) {
        return -1;
    }

    // Create a context on the default device and a command queue.
    cl::Context context(CL_DEVICE_TYPE_DEFAULT);
    cl::CommandQueue queue(context);

    // Optimized kernel:
    // - We pass the lengths so each work-item knows the search bounds.
    // - The substring is marked __constant so that it is cached.
    // - Each thread only runs if it can fit the entire substring.
    // - When a match is found, atomic_min updates the global result.
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

    // Create buffers. We don’t need to copy a null terminator because
    // the kernel uses the provided lengths.
    cl::Buffer d_str(context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR,
                     str.size() * sizeof(char), (void*)str.data());
    cl::Buffer d_substr(context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR,
                        substr.size() * sizeof(char), (void*)substr.data());
    cl::Buffer d_result(context, CL_MEM_READ_WRITE, sizeof(int));

    // Initialize the result to a high value (so that atomic_min works).
    int initResult = std::numeric_limits<int>::max();
    queue.enqueueWriteBuffer(d_result, CL_TRUE, 0, sizeof(int), &initResult);

    // Create the kernel and set its arguments.
    cl::Kernel kernel(program, "search");
    kernel.setArg(0, d_str);
    kernel.setArg(1, d_substr);
    kernel.setArg(2, d_result);
    int str_length  = static_cast<int>(str.size());
    int substr_length = static_cast<int>(substr.size());
    kernel.setArg(3, str_length);
    kernel.setArg(4, substr_length);

    // Launch one thread per valid starting position.
    size_t globalSize = str.size() - substr.size() + 1;
    {
        PROFILE_SCOPE("CL Search");
        queue.enqueueNDRangeKernel(kernel, cl::NullRange, cl::NDRange(globalSize));
        queue.finish();
    }

    // Read back the result.
    int result;
    queue.enqueueReadBuffer(d_result, CL_TRUE, 0, sizeof(int), &result);
    if (result == std::numeric_limits<int>::max()) {
        result = -1; // No match found.
    }
    return result;
}
