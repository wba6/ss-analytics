#include <algorithm>
#include <cstdint>
#include <iterator>
#include <vulkan/vulkan.hpp>
#include <cstring>
#include <cassert>
#include <iostream>
#include "performance-analyzer/performance-analyzer.hpp"
#include "vulkan/vulkan_core.h"
#include "vulkan/vulkan_enums.hpp"
#include "vulkan/vulkan_handles.hpp"
#include "vulkan/vulkan_structs.hpp"
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

void vulkanSqaure(const int number) {

    const uint32_t NumElements = 10;
    const uint32_t BufferSize = NumElements * sizeof(int32_t);

   //Fill application info
    vk::ApplicationInfo AppInfo{
        "vulkanCompute",    // Application name
        1,                  // Application Version
        nullptr,            // Engine name or nullptr
        0,                  // Engine Version
        VK_API_VERSION_1_1  // Vulkan API Version
    };

    // Enable validation layer
    const std::vector<const char*> Layers = { "VK_LAYER_KHRONOS_validation" };
    vk::InstanceCreateInfo InstanceCreateInfo (
            vk::InstanceCreateFlags(),    // Flags
            &AppInfo,                   // Application info
            Layers.size(),              // Layers Count
            Layers.data()               // Layers
    );
    vk::Instance Instance = vk::createInstance(InstanceCreateInfo);

    // Determine physical device
    vk::PhysicalDevice PhysicalDevice = Instance.enumeratePhysicalDevices().front();
    vk::PhysicalDeviceProperties DeviceProps = PhysicalDevice.getProperties();
    std::cout << "Vulkan device name: " << DeviceProps.deviceName << std::endl;
    const uint32_t ApiVersion = DeviceProps.apiVersion;
    std::cout << "Using API version: " << VK_VERSION_MAJOR(ApiVersion)
            << "." << VK_VERSION_MINOR(ApiVersion) << "." << VK_VERSION_PATCH(ApiVersion);

    // Queue Family 
    auto QueueFamilyProps = PhysicalDevice.getQueueFamilyProperties();
    auto PropIt = std::find_if(
            QueueFamilyProps.begin(), QueueFamilyProps.end(),
            [](const vk::QueueFamilyProperties& Prop){
                return Prop.queueFlags & vk::QueueFlagBits::eCompute;
            });
    const uint32_t ComputeQueueFamilyIndex = std::distance(QueueFamilyProps.begin(), PropIt);
    std::cout << "Compute Queue Family Index: " << ComputeQueueFamilyIndex << std::endl;

    // Create device
    vk::DeviceQueueCreateInfo DeviceQueueCreateInfo(
            vk::DeviceQueueCreateFlags(),   // Flags
            ComputeQueueFamilyIndex,        // Queue family index
            1                               // Number of Queues
     );
    vk::DeviceCreateInfo DeviceCreateInfo(
            vk::DeviceCreateFlags(),
            DeviceQueueCreateInfo
    );
    vk::Device Device = PhysicalDevice.createDevice(DeviceCreateInfo);


    // Buffer Creation
    vk::BufferCreateInfo BufferCreateInfo{
        vk::BufferCreateFlags(),
        NumElements * sizeof(int32_t),      // size
        vk::BufferUsageFlagBits::eStorageBuffer, // Usage
        vk::SharingMode::eExclusive,            // SharingMode
        1,                                  // Numver of queue family indices
        &ComputeQueueFamilyIndex            // List of queue family indices
    };
    vk::Buffer InBuffer = Device.createBuffer(BufferCreateInfo);
    vk::Buffer OutBuffer = Device.createBuffer(BufferCreateInfo);

    // Queue Memory Types
    vk::PhysicalDeviceMemoryProperties MemoryProperties = PhysicalDevice.getMemoryProperties();
    uint32_t MemoryTypeIndex = -1;
    for(uint32_t i = 0; i < MemoryProperties.memoryTypeCount; ++i) {
        vk::MemoryType MemoryType = MemoryProperties.memoryTypes[i];
        if ((vk::MemoryPropertyFlagBits::eHostVisible & MemoryType.propertyFlags) &&
            (vk::MemoryPropertyFlagBits::eHostCoherent & MemoryType.propertyFlags)) {
            MemoryTypeIndex = i;
            break;
        }
    }

    // Memory requirements
    vk::MemoryRequirements InBufferMemoryRequirements = Device.getBufferMemoryRequirements(InBuffer);
    vk::MemoryRequirements OutBufferMemoryRequirements = Device.getBufferMemoryRequirements(OutBuffer);

    // Allocate memory
    vk::MemoryAllocateInfo InBufferMemoryAllocateInfo(InBufferMemoryRequirements.size, MemoryTypeIndex);
    vk::MemoryAllocateInfo OutBufferMemoryAllocateInfo(OutBufferMemoryRequirements.size, MemoryTypeIndex);
    vk::DeviceMemory InBufferMemory = Device.allocateMemory(InBufferMemoryAllocateInfo);
    vk::DeviceMemory OutBufferMemory = Device.allocateMemory(OutBufferMemoryAllocateInfo);

    // Map Memory and write
    int32_t* InBufferPtr = 
        static_cast<int32_t*>(Device.mapMemory(InBufferMemory, 0, BufferSize));
    for (int32_t i = 0; i < NumElements; ++i) {
        InBufferPtr[i] = i;
    }
    Device.unmapMemory(InBufferMemory);

    // Bind Buffers to Memory
    Device.bindBufferMemory(InBuffer, InBufferMemory, 0);
    Device.bindBufferMemory(OutBuffer, OutBufferMemory, 0);

    // Create pipline for compute shader 14:56


}


