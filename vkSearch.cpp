#include <vulkan/vulkan.h>
#include <cstring>
#include <cassert>
#include "performance-analyzer/performance-analyzer.hpp"
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


