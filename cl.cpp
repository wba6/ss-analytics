#include "vendor/performance-analyzer/Profiler.h"
#include "vendor/performance-analyzer/Timer.h"
#include <ctime>
#include <iostream>
#define CL_HPP_TARGET_OPENCL_VERSION 300
#define CL_HPP_ENABLE_EXCEPTIONS
#include <string>
#include <CL/opencl.hpp>
#include "performance-analyzer/Profiler.h"

//This function can throw exceptions
int clSearch(const std::string& str,const std::string& substr){
    PROFILE_FUNCTION();

    // Grab context
    cl::Context context(CL_DEVICE_TYPE_DEFAULT);

    // Create a command queue
    cl::CommandQueue queue(context);

    std::string kernalSource = R"(
        __kernel void search(__global const char* str, __global const char* substr, __global int* result){
            int i = get_global_id(0);
            if (str[i] == substr[0]){
                int j = 0;
                while (substr[j] != '\0'){
                    if (str[i+j] != substr[j]){
                        break;
                    }
                    j++;
                }
                if (substr[j] == '\0'){
                    *result = i;
                }
            }
        }
    )";
    Timer* timer = new Timer("Buffers");
    cl::Program program(context,kernalSource,true);
    program.build();
    delete timer;

    timer = new Timer("Buffers");
    //data buffers
    cl::Buffer d_str(context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, str.size() + 1, (void*)str.c_str());
    cl::Buffer d_substr(context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, substr.size() + 1, (void*)substr.c_str());
    cl::Buffer d_result(context, CL_MEM_WRITE_ONLY, sizeof(int));
    delete timer;

    //create kernel
    cl::compatibility::make_kernel<cl::Buffer,cl::Buffer,cl::Buffer> kernel(program, "search");
    {

        PROFILE_SCOPE("Build Program");


        //run kernel
        kernel(cl::EnqueueArgs(queue,cl::NDRange(str.size())),d_str,d_substr,d_result);
    }
        //read result
        int result = -1;
        cl::copy(queue, d_result,&result,&result + 1);
        return result;
}
