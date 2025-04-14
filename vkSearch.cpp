#include <algorithm>
#include <cstddef>
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

void vulkanSquare() {

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

    // Define validation layers (only)
    const std::vector<const char*> Layers = { 
        "VK_LAYER_KHRONOS_validation" 
    };

    // Define instance extensions (include the portability extension here)
    const std::vector<const char*> Extensions = {
        "VK_KHR_portability_enumeration"
    };

    // Enable validation layer
    vk::InstanceCreateInfo InstanceCreateInfo (
            vk::InstanceCreateFlags(vk::InstanceCreateFlagBits::eEnumeratePortabilityKHR),    // Flags
            &AppInfo,                   // Application info
            Layers.size(),              // Layers Count
            Layers.data(),               // Layers
            Extensions.size(),
            Extensions.data()
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

    //ShaderContents contains the data in .spv shader file
    std::string ShaderContents = R"(
            #version 430
            layout(local_size_x = 1, local_size_y = 1) in;

            layout(std430, binding = 0) buffer lay0 {int inbuf[]; };
            layout(std430, binding = 1) buffer lay1 {int outbuf[]; };

            void main() {
                const uint id = gl_GlobalInvocationID.x;

                outbuf[id] = inbuf[id] * inbuf[id];
            }
        )";

    const size_t ShaderContentsSize = ShaderContents.size();
    const uint32_t* ShaderContentsData = (const uint32_t*) ShaderContents.data();
    vk::ShaderModuleCreateInfo ShaderModuleCreateInfo(
            vk::ShaderModuleCreateFlags(),      //Flags
            ShaderContentsSize,                 // code size
            ShaderContentsData                  // code
    );
    vk::ShaderModule ShaderModule = Device.createShaderModule(ShaderModuleCreateInfo);

    // Descriptor set layout
    const std::vector<vk::DescriptorSetLayoutBinding> DescriptorSetLayoutBinding = {
        {0, vk::DescriptorType::eStorageBuffer, 1, vk::ShaderStageFlagBits::eCompute},
        {1, vk::DescriptorType::eStorageBuffer, 1, vk::ShaderStageFlagBits::eCompute}
    };
    vk::DescriptorSetLayoutCreateInfo DescriptorSetLayoutCreateInfo (
            vk::DescriptorSetLayoutCreateFlags(),
            DescriptorSetLayoutBinding
    );
    vk::DescriptorSetLayout DescriptorSetLayout = Device.createDescriptorSetLayout(DescriptorSetLayoutCreateInfo);


    // pipline Layout 
    vk::PipelineLayoutCreateInfo PipelineLayoutCreateInfo (
            vk::PipelineLayoutCreateFlags(),
            DescriptorSetLayout
    );
    vk::PipelineLayout PipelineLayout = Device.createPipelineLayout(PipelineLayoutCreateInfo);
    vk::PipelineCache PipelineCache = Device.createPipelineCache(vk::PipelineCacheCreateInfo());

    // Pipeline
    vk::PipelineShaderStageCreateInfo PipelineShaderCreateInfo(
            vk::PipelineShaderStageCreateFlags(),       // Flags
            vk::ShaderStageFlagBits::eCompute,          // stage 
            ShaderModule,                               // Shader module 
            "main"                                      // Shader entry point
    );
    vk::ComputePipelineCreateInfo ComputePipelineCreateInfo(
            vk::PipelineCreateFlags(),      // flags 
            PipelineShaderCreateInfo,       // shader create info struct
            PipelineLayout                  // pipeline layout
    );
    vk::Pipeline ComputePipeline = Device.createComputePipeline(PipelineCache, ComputePipelineCreateInfo).value;

    // Descriptor pool
    vk::DescriptorPoolSize DescriptorPoolSize(
            vk::DescriptorType::eStorageBuffer,
            2
    );
    vk::DescriptorPoolCreateInfo DescriptorPoolCreateInfo(
            vk::DescriptorPoolCreateFlags(),
            1,
            DescriptorPoolSize
    );
    vk::DescriptorPool DescriptorPool = Device.createDescriptorPool(DescriptorPoolCreateInfo);

    // Descriptor sets 
    vk::DescriptorSetAllocateInfo allocInfo(DescriptorPool, 1, &DescriptorSetLayout);
    auto DescriptorSets = Device.allocateDescriptorSets(allocInfo);
    vk::DescriptorSet DescriptorSet = DescriptorSets.front();
    vk::DescriptorBufferInfo InBufferInfo(InBuffer, 0, NumElements * sizeof(int32_t));
    vk::DescriptorBufferInfo OutBufferInfo(OutBuffer, 0, NumElements * sizeof(int32_t));

    // update descriptor sets to use out Buffers
    const std::vector<vk::WriteDescriptorSet> WriteDescriptorSets = {
        {DescriptorSet, 0,0,1, vk::DescriptorType::eStorageBuffer, nullptr, &InBufferInfo},
        {DescriptorSet, 1,0,1, vk::DescriptorType::eStorageBuffer, nullptr, &OutBufferInfo}
    };
    Device.updateDescriptorSets(WriteDescriptorSets, {});


    // Submit work to the cpu 

    vk::CommandPoolCreateInfo CommandPoolCreateInfo(
            vk::CommandPoolCreateFlags(),
            ComputeQueueFamilyIndex
    );
    vk::CommandPool CommandPool = Device.createCommandPool(CommandPoolCreateInfo);

    vk::CommandBufferAllocateInfo CommandBufferAllocInfo(
            CommandPool, 
            vk::CommandBufferLevel::ePrimary,
            1
    );
    auto CmdBuffers = Device.allocateCommandBuffers(CommandBufferAllocInfo);
    vk::CommandBuffer CmdBuffer = CmdBuffers.front();

    // Record commands 17:26
    vk::CommandBufferBeginInfo CmdBufferBeginInfo(vk::CommandBufferUsageFlagBits::eOneTimeSubmit);
    CmdBuffer.begin(CmdBufferBeginInfo);
    CmdBuffer.bindPipeline(vk::PipelineBindPoint::eCompute, ComputePipeline);
    CmdBuffer.bindDescriptorSets(
            vk::PipelineBindPoint::eCompute,
            PipelineLayout,
            0,
            { DescriptorSet },
            {}
    );
    CmdBuffer.dispatch(NumElements, 1, 1);
    CmdBuffer.end();

    // Submit work and wait
    vk::Queue Queue = Device.getQueue(ComputeQueueFamilyIndex, 0);
    vk::Fence Fence = Device.createFence(vk::FenceCreateInfo());

    vk::SubmitInfo SubmitInfo(
            0,
            nullptr,
            nullptr,
            1,
            &CmdBuffer
    );
    Queue.submit({ SubmitInfo }, Fence);
    Device.waitForFences(
            {Fence},
            true,
            uint64_t(-1)
    );

    // Read output
    int32_t* OutBufferPtr = static_cast<int32_t*>(Device.mapMemory(OutBufferMemory, 0, BufferSize));
    for(uint32_t i = 0; i < NumElements; ++i) {
        std::cout << OutBufferPtr[i] << " ";
    }
    std::cout << std::endl;
    Device.unmapMemory(OutBufferMemory);

    Device.freeMemory(OutBufferMemory);
    Device.destroyBuffer(OutBuffer);
    Device.destroy();
}


