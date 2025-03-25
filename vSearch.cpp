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
    // Convert input strings to vectors of uint32_t (each char stored as its ASCII code)
    std::vector<uint32_t> textData(haystack.size());
    for (size_t i = 0; i < haystack.size(); i++) {
        textData[i] = static_cast<uint32_t>(haystack[i]);
    }
    std::vector<uint32_t> patternData(needle.size());
    for (size_t i = 0; i < needle.size(); i++) {
        patternData[i] = static_cast<uint32_t>(needle[i]);
    }
    uint32_t textLength = static_cast<uint32_t>(textData.size());
    uint32_t patternLength = static_cast<uint32_t>(patternData.size());
    // Use a very high initial value so that atomicMin can later record a lower index.
    int initialResult = 0x7fffffff;

    // ----------- Create Vulkan Instance -----------
    VkInstance instance;
    {
        VkApplicationInfo appInfo = {};
        appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        appInfo.pApplicationName = "VulkanStringSearch";
        appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
        appInfo.pEngineName = "NoEngine";
        appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
        appInfo.apiVersion = VK_API_VERSION_1_1;
    
        VkInstanceCreateInfo createInfo = {};
        createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        createInfo.pApplicationInfo = &appInfo;
    
        VK_CHECK(vkCreateInstance(&createInfo, nullptr, &instance));
    }

    // ----------- Select a Physical Device that Supports Compute -----------
    VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
    {
        uint32_t deviceCount = 0;
        vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);
        if (deviceCount == 0)
            throw std::runtime_error("No Vulkan devices found.");
        std::vector<VkPhysicalDevice> devices(deviceCount);
        vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());
        for (auto dev : devices) {
            uint32_t queueFamilyCount = 0;
            vkGetPhysicalDeviceQueueFamilyProperties(dev, &queueFamilyCount, nullptr);
            std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
            vkGetPhysicalDeviceQueueFamilyProperties(dev, &queueFamilyCount, queueFamilies.data());
            for (uint32_t i = 0; i < queueFamilyCount; i++) {
                if (queueFamilies[i].queueFlags & VK_QUEUE_COMPUTE_BIT) {
                    physicalDevice = dev;
                    break;
                }
            }
            if (physicalDevice != VK_NULL_HANDLE)
                break;
        }
        if (physicalDevice == VK_NULL_HANDLE)
            throw std::runtime_error("No device with compute capability found.");
    }

    // ----------- Create Logical Device and Compute Queue -----------
    VkDevice device;
    VkQueue computeQueue;
    uint32_t computeQueueFamilyIndex = 0;
    {
        uint32_t queueFamilyCount = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, nullptr);
        std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
        vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, queueFamilies.data());
        bool found = false;
        for (uint32_t i = 0; i < queueFamilyCount; i++) {
            if (queueFamilies[i].queueFlags & VK_QUEUE_COMPUTE_BIT) {
                computeQueueFamilyIndex = i;
                found = true;
                break;
            }
        }
        if (!found)
            throw std::runtime_error("Failed to find a compute queue family.");
    
        float queuePriority = 1.0f;
        VkDeviceQueueCreateInfo queueCreateInfo = {};
        queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueCreateInfo.queueFamilyIndex = computeQueueFamilyIndex;
        queueCreateInfo.queueCount = 1;
        queueCreateInfo.pQueuePriorities = &queuePriority;
    
        VkDeviceCreateInfo deviceCreateInfo = {};
        deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        deviceCreateInfo.queueCreateInfoCount = 1;
        deviceCreateInfo.pQueueCreateInfos = &queueCreateInfo;
    
        VK_CHECK(vkCreateDevice(physicalDevice, &deviceCreateInfo, nullptr, &device));
        vkGetDeviceQueue(device, computeQueueFamilyIndex, 0, &computeQueue);
    }

    // ----------- Helper: Create a Buffer with Host-Visible Memory -----------
    auto createBuffer = [&](VkDeviceSize size, VkBufferUsageFlags usage,
                              VkBuffer &buffer, VkDeviceMemory &memory) {
        VkBufferCreateInfo bufferInfo = {};
        bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bufferInfo.size = size;
        bufferInfo.usage = usage;
        bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        VK_CHECK(vkCreateBuffer(device, &bufferInfo, nullptr, &buffer));
    
        VkMemoryRequirements memRequirements;
        vkGetBufferMemoryRequirements(device, buffer, &memRequirements);
    
        VkPhysicalDeviceMemoryProperties memProperties;
        vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);
        uint32_t memoryTypeIndex = 0;
        bool memFound = false;
        for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
            if ((memRequirements.memoryTypeBits & (1 << i)) &&
                (memProperties.memoryTypes[i].propertyFlags &
                 (VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)) ==
                (VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)) {
                memoryTypeIndex = i;
                memFound = true;
                break;
            }
        }
        if (!memFound)
            throw std::runtime_error("Failed to find suitable memory type.");
    
        VkMemoryAllocateInfo allocInfo = {};
        allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.allocationSize = memRequirements.size;
        allocInfo.memoryTypeIndex = memoryTypeIndex;
    
        VK_CHECK(vkAllocateMemory(device, &allocInfo, nullptr, &memory));
        vkBindBufferMemory(device, buffer, memory, 0);
    };

    // ----------- Create Buffers for the Text, Pattern, and the Result -----------
    VkBuffer textBuffer, patternBuffer, resultBuffer;
    VkDeviceMemory textMemory, patternMemory, resultMemory;
    createBuffer(sizeof(uint32_t) * textData.size(), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                 textBuffer, textMemory);
    createBuffer(sizeof(uint32_t) * patternData.size(), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                 patternBuffer, patternMemory);
    createBuffer(sizeof(int), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, resultBuffer, resultMemory);
    
    // Map and copy input data to the buffers.
    {
        void* data;
        vkMapMemory(device, textMemory, 0, VK_WHOLE_SIZE, 0, &data);
        memcpy(data, textData.data(), sizeof(uint32_t) * textData.size());
        vkUnmapMemory(device, textMemory);
    
        vkMapMemory(device, patternMemory, 0, VK_WHOLE_SIZE, 0, &data);
        memcpy(data, patternData.data(), sizeof(uint32_t) * patternData.size());
        vkUnmapMemory(device, patternMemory);
    
        vkMapMemory(device, resultMemory, 0, sizeof(int), 0, &data);
        memcpy(data, &initialResult, sizeof(int));
        vkUnmapMemory(device, resultMemory);
    }
    
    // ----------- Set Up Descriptor Set, Pipeline Layout, and Compute Pipeline -----------
    VkDescriptorSetLayout descriptorSetLayout;
    VkPipelineLayout pipelineLayout;
    VkPipeline computePipeline;
    VkDescriptorPool descriptorPool;
    VkDescriptorSet descriptorSet;
    {
        // Three bindings: text (binding 0), pattern (binding 1), result (binding 2)
        VkDescriptorSetLayoutBinding bindings[3] = {};
        for (uint32_t i = 0; i < 3; i++) {
            bindings[i].binding = i;
            bindings[i].descriptorCount = 1;
            bindings[i].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
            bindings[i].pImmutableSamplers = nullptr;
            bindings[i].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
        }
        VkDescriptorSetLayoutCreateInfo dsLayoutInfo = {};
        dsLayoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        dsLayoutInfo.bindingCount = 3;
        dsLayoutInfo.pBindings = bindings;
        VK_CHECK(vkCreateDescriptorSetLayout(device, &dsLayoutInfo, nullptr, &descriptorSetLayout));
    
        // Define a push constant range for textLength and patternLength.
        VkPushConstantRange pushConstantRange = {};
        pushConstantRange.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
        pushConstantRange.offset = 0;
        pushConstantRange.size = sizeof(uint32_t) * 2;
    
        VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
        pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipelineLayoutInfo.setLayoutCount = 1;
        pipelineLayoutInfo.pSetLayouts = &descriptorSetLayout;
        pipelineLayoutInfo.pushConstantRangeCount = 1;
        pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;
        VK_CHECK(vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &pipelineLayout));
    
        // Load precompiled SPIR-V binary for the compute shader.
        // (Replace the placeholder below with your actual SPIR-V binary data.)
        const uint32_t shaderCode[] = {
            // <-- Precompiled SPIR-V binary data goes here -->
        };
        size_t shaderSize = sizeof(shaderCode);
    
        VkShaderModuleCreateInfo shaderModuleInfo = {};
        shaderModuleInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        shaderModuleInfo.codeSize = shaderSize;
        shaderModuleInfo.pCode = shaderCode;
        VkShaderModule shaderModule;
        VK_CHECK(vkCreateShaderModule(device, &shaderModuleInfo, nullptr, &shaderModule));
    
        VkPipelineShaderStageCreateInfo shaderStageInfo = {};
        shaderStageInfo.sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        shaderStageInfo.stage  = VK_SHADER_STAGE_COMPUTE_BIT;
        shaderStageInfo.module = shaderModule;
        shaderStageInfo.pName  = "main";
    
        VkComputePipelineCreateInfo pipelineInfo = {};
        pipelineInfo.sType  = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
        pipelineInfo.stage  = shaderStageInfo;
        pipelineInfo.layout = pipelineLayout;
        VK_CHECK(vkCreateComputePipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &computePipeline));
    
        vkDestroyShaderModule(device, shaderModule, nullptr);
    
        // Create descriptor pool and allocate a descriptor set.
        VkDescriptorPoolSize poolSize = {};
        poolSize.type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        poolSize.descriptorCount = 3;
        VkDescriptorPoolCreateInfo poolInfo = {};
        poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        poolInfo.maxSets = 1;
        poolInfo.poolSizeCount = 1;
        poolInfo.pPoolSizes = &poolSize;
        VK_CHECK(vkCreateDescriptorPool(device, &poolInfo, nullptr, &descriptorPool));
    
        VkDescriptorSetAllocateInfo allocInfo = {};
        allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        allocInfo.descriptorPool = descriptorPool;
        allocInfo.descriptorSetCount = 1;
        allocInfo.pSetLayouts = &descriptorSetLayout;
        VK_CHECK(vkAllocateDescriptorSets(device, &allocInfo, &descriptorSet));
    
        VkDescriptorBufferInfo textBufferInfo    = { textBuffer, 0, VK_WHOLE_SIZE };
        VkDescriptorBufferInfo patternBufferInfo = { patternBuffer, 0, VK_WHOLE_SIZE };
        VkDescriptorBufferInfo resultBufferInfo  = { resultBuffer, 0, VK_WHOLE_SIZE };
        VkWriteDescriptorSet descriptorWrites[3] = {};
        descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites[0].dstSet = descriptorSet;
        descriptorWrites[0].dstBinding = 0;
        descriptorWrites[0].descriptorCount = 1;
        descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        descriptorWrites[0].pBufferInfo = &textBufferInfo;
    
        descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites[1].dstSet = descriptorSet;
        descriptorWrites[1].dstBinding = 1;
        descriptorWrites[1].descriptorCount = 1;
        descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        descriptorWrites[1].pBufferInfo = &patternBufferInfo;
    
        descriptorWrites[2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites[2].dstSet = descriptorSet;
        descriptorWrites[2].dstBinding = 2;
        descriptorWrites[2].descriptorCount = 1;
        descriptorWrites[2].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        descriptorWrites[2].pBufferInfo = &resultBufferInfo;
    
        vkUpdateDescriptorSets(device, 3, descriptorWrites, 0, nullptr);
    }
    
    // ----------- Record and Submit Command Buffer -----------
    VkCommandPool commandPool;
    VkCommandBuffer commandBuffer;
    {
        VkCommandPoolCreateInfo poolInfo = {};
        poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        poolInfo.queueFamilyIndex = computeQueueFamilyIndex;
        VK_CHECK(vkCreateCommandPool(device, &poolInfo, nullptr, &commandPool));
    
        VkCommandBufferAllocateInfo allocInfo = {};
        allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocInfo.commandPool = commandPool;
        allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocInfo.commandBufferCount = 1;
        VK_CHECK(vkAllocateCommandBuffers(device, &allocInfo, &commandBuffer));
    
        VkCommandBufferBeginInfo beginInfo = {};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        VK_CHECK(vkBeginCommandBuffer(commandBuffer, &beginInfo));
    
        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, computePipeline);
        vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipelineLayout,
                                0, 1, &descriptorSet, 0, nullptr);
        // Pass textLength and patternLength as push constants.
        uint32_t pushConstants[2] = { textLength, patternLength };
        vkCmdPushConstants(commandBuffer, pipelineLayout, VK_SHADER_STAGE_COMPUTE_BIT,
                           0, sizeof(pushConstants), pushConstants);
    
        // Dispatch workgroups so that each invocation tests one possible starting index.
        // (Assuming local_size_x = 256 as in the shader.)
        uint32_t workgroupCount = (textLength - patternLength + 256) / 256;
        vkCmdDispatch(commandBuffer, workgroupCount, 1, 1);
    
        VK_CHECK(vkEndCommandBuffer(commandBuffer));
    
        VkSubmitInfo submitInfo = {};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &commandBuffer;
        VK_CHECK(vkQueueSubmit(computeQueue, 1, &submitInfo, VK_NULL_HANDLE));
        VK_CHECK(vkQueueWaitIdle(computeQueue));
    }
    
    // ----------- Read Back the Result -----------
    int foundIndex = -1;
    {
        void* mapped;
        vkMapMemory(device, resultMemory, 0, sizeof(int), 0, &mapped);
        memcpy(&foundIndex, mapped, sizeof(int));
        vkUnmapMemory(device, resultMemory);
        if (foundIndex == 0x7fffffff)
            foundIndex = -1; // Not found.
    }
    
    // ----------- Cleanup All Vulkan Resources -----------
    vkDestroyCommandPool(device, commandPool, nullptr);
    vkDestroyDescriptorPool(device, descriptorPool, nullptr);
    vkDestroyPipeline(device, computePipeline, nullptr);
    vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
    vkDestroyDescriptorSetLayout(device, descriptorSetLayout, nullptr);
    vkDestroyBuffer(device, textBuffer, nullptr);
    vkDestroyBuffer(device, patternBuffer, nullptr);
    vkDestroyBuffer(device, resultBuffer, nullptr);
    vkFreeMemory(device, textMemory, nullptr);
    vkFreeMemory(device, patternMemory, nullptr);
    vkFreeMemory(device, resultMemory, nullptr);
    vkDestroyDevice(device, nullptr);
    vkDestroyInstance(instance, nullptr);
    
    return foundIndex;
}


