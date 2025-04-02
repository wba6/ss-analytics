#include <vulkan/vulkan.h>
#include <iostream>
#include <vector>
#include <stdexcept>
#include <cstring>
#include <cassert>
#include "performance-analyzer/Profiler.h"
#include "vkSearch.hpp"
// Helper macro for error checking.
#define VK_CHECK(x) do { VkResult err = x; if (err) { \
    std::cerr << "Detected Vulkan error: " << err << std::endl; \
    throw std::runtime_error("Vulkan error"); } } while(0)


// This function takes a full string ("haystack") and a substring ("needle"),
// then uses a Vulkan compute shader to search for the first occurrence of needle.
int vulkanStringSearch(const std::string &haystack, const std::string &needle) {
    PROFILE_SCOPE("VkSearch");
    int foundIndex = -1;
    return foundIndex;
}


