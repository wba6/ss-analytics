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

// A helper function to load a .spv file off disk:
std::vector<char> LoadShaderFile(const std::string& filename) {
    std::vector<char> buffer;
    std::ifstream file(filename, std::ios::binary | std::ios::ate);
    if (file) {
        const size_t fileSize = file.tellg();
        file.seekg(0);
        buffer.resize(fileSize);
        file.read(buffer.data(), fileSize);
    }
    return buffer;
}

struct PushConsts {
    uint32_t textLength;
    uint32_t patternLength;
};

// This function takes a full string ("haystack") and a substring ("needle"),
// then uses a Vulkan compute shader to search for the first occurrence of needle.
int vulkanStringSearch(const std::string &haystack, const std::string &needle) {

    // PROFILE_SCOPE("VkSearch"); // Assuming a macro for performance profiling

    ////////////////////////////////////////////////////////////////////////
    //                          VULKAN INSTANCE                           //
    ////////////////////////////////////////////////////////////////////////
    vk::ApplicationInfo appInfo{
        "VulkanSubstringSearch", // Application Name
        1,                       // Application Version
        nullptr,                 // Engine Name or nullptr
        0,                       // Engine Version
        VK_API_VERSION_1_1       // Vulkan API Version
    };

    const std::vector<const char*> layers = {
        "VK_LAYER_KHRONOS_validation"
    };

    const std::vector<const char*> extensions = {
        "VK_KHR_portability_enumeration"
    };

    vk::InstanceCreateInfo instanceCreateInfo(
        vk::InstanceCreateFlags(vk::InstanceCreateFlagBits::eEnumeratePortabilityKHR),
        &appInfo,
        static_cast<uint32_t>(layers.size()),
        layers.data(),
        static_cast<uint32_t>(extensions.size()),
        extensions.data()
    );
    vk::Instance instance = vk::createInstance(instanceCreateInfo);

    ////////////////////////////////////////////////////////////////////////
    //                          PHYSICAL DEVICE                           //
    ////////////////////////////////////////////////////////////////////////
    auto physicalDevices = instance.enumeratePhysicalDevices();
    if (physicalDevices.empty())
        throw std::runtime_error("No Vulkan-capable device found!");
    vk::PhysicalDevice physicalDevice = physicalDevices.front();

    vk::PhysicalDeviceProperties deviceProps = physicalDevice.getProperties();
    std::cout << "Using device: " << deviceProps.deviceName << std::endl;
    const uint32_t apiVersion = deviceProps.apiVersion;
    std::cout << "Vulkan API version: " 
              << VK_VERSION_MAJOR(apiVersion) << "." 
              << VK_VERSION_MINOR(apiVersion) << "." 
              << VK_VERSION_PATCH(apiVersion) << std::endl;

    ////////////////////////////////////////////////////////////////////////
    //                            QUEUE FAMILY                            //
    ////////////////////////////////////////////////////////////////////////
    std::vector<vk::QueueFamilyProperties> queueFamilyProps = physicalDevice.getQueueFamilyProperties();
    auto it = std::find_if(queueFamilyProps.begin(), queueFamilyProps.end(),
        [](const vk::QueueFamilyProperties& qfp) {
            return qfp.queueFlags & vk::QueueFlagBits::eCompute;
        }
    );
    if (it == queueFamilyProps.end())
        throw std::runtime_error("No compute queue family found!");
    uint32_t computeQueueFamilyIndex = static_cast<uint32_t>(std::distance(queueFamilyProps.begin(), it));
    std::cout << "Compute Queue Family Index: " << computeQueueFamilyIndex << std::endl;

    ////////////////////////////////////////////////////////////////////////
    //                               DEVICE                               //
    ////////////////////////////////////////////////////////////////////////
    
    float queuePriorities = 1.0f;
       const std::vector<const char*> deviceExtensions = {
        "VK_KHR_portability_subset",
        "VK_KHR_8bit_storage"
    };
    // Create the device queue create info as before.
    vk::DeviceQueueCreateInfo deviceQueueCreateInfo(
        vk::DeviceQueueCreateFlags(),
        computeQueueFamilyIndex,
        1,
        &queuePriorities
    );

    // Set up the 8-bit storage features.
    vk::PhysicalDevice8BitStorageFeatures eightBitStorageFeatures{};
    eightBitStorageFeatures.sType = vk::StructureType::ePhysicalDevice8BitStorageFeatures;
    eightBitStorageFeatures.pNext = nullptr;
    eightBitStorageFeatures.uniformAndStorageBuffer8BitAccess = VK_TRUE;  // Enable the feature.

    // Now create the device. Chain the eightBitStorageFeatures in the pNext field.
    vk::DeviceCreateInfo deviceCreateInfo(
        vk::DeviceCreateFlags(),
        1,
        &deviceQueueCreateInfo,
        0,
        nullptr,
        static_cast<uint32_t>(deviceExtensions.size()),
        deviceExtensions.data()
    );

    // Chain in the 8-bit storage features.
    deviceCreateInfo.pNext = &eightBitStorageFeatures;

    // Create the device.
    vk::Device device = physicalDevice.createDevice(deviceCreateInfo);
    ////////////////////////////////////////////////////////////////////////
    //                         ALLOCATING MEMORY                          //
    ////////////////////////////////////////////////////////////////////////
    // Define our search text and pattern.
    std::string text = "Vulkan is a low-overhead, cross-platform API for high-performance graphics and computation. "
                       "This is a naive substring search example in Vulkan. The word 'Vulkan' appears multiple times. Vulkan!";
    std::string pattern = "Vulkan";

    // We'll allow up to 100 occurrences.
    const uint32_t maxOccurrences = 100;

    // Create text buffer.
    vk::DeviceSize textBufferSize = text.size();
    vk::BufferCreateInfo textBufferCreateInfo(
        vk::BufferCreateFlags(),
        textBufferSize,
        vk::BufferUsageFlagBits::eStorageBuffer,
        vk::SharingMode::eExclusive,
        1,
        &computeQueueFamilyIndex
    );
    vk::Buffer textBuffer = device.createBuffer(textBufferCreateInfo);

    // Create pattern buffer.
    vk::DeviceSize patternBufferSize = pattern.size();
    vk::BufferCreateInfo patternBufferCreateInfo(
        vk::BufferCreateFlags(),
        patternBufferSize,
        vk::BufferUsageFlagBits::eStorageBuffer,
        vk::SharingMode::eExclusive,
        1,
        &computeQueueFamilyIndex
    );
    vk::Buffer patternBuffer = device.createBuffer(patternBufferCreateInfo);

    // Create results buffer.
    vk::DeviceSize resultsBufferSize = maxOccurrences * sizeof(int32_t);
    vk::BufferCreateInfo resultsBufferCreateInfo(
        vk::BufferCreateFlags(),
        resultsBufferSize,
        vk::BufferUsageFlagBits::eStorageBuffer,
        vk::SharingMode::eExclusive,
        1,
        &computeQueueFamilyIndex
    );
    vk::Buffer resultsBuffer = device.createBuffer(resultsBufferCreateInfo);

    // Get memory requirements for each buffer.
    vk::MemoryRequirements textMemReq = device.getBufferMemoryRequirements(textBuffer);
    vk::MemoryRequirements patternMemReq = device.getBufferMemoryRequirements(patternBuffer);
    vk::MemoryRequirements resultsMemReq = device.getBufferMemoryRequirements(resultsBuffer);

    // Determine a suitable memory type.
    vk::PhysicalDeviceMemoryProperties memProps = physicalDevice.getMemoryProperties();
    uint32_t memoryTypeIndex = uint32_t(~0);
    for (uint32_t i = 0; i < memProps.memoryTypeCount; i++) {
        const vk::MemoryType& memType = memProps.memoryTypes[i];
        if ((memType.propertyFlags & vk::MemoryPropertyFlagBits::eHostVisible) &&
            (memType.propertyFlags & vk::MemoryPropertyFlagBits::eHostCoherent)) 
        {
            memoryTypeIndex = i;
            break;
        }
    }
    if (memoryTypeIndex == uint32_t(~0))
        throw std::runtime_error("Failed to find suitable memory type!");

    // Allocate and bind memory for text buffer.
    vk::MemoryAllocateInfo textMemAlloc(textMemReq.size, memoryTypeIndex);
    vk::DeviceMemory textMemory = device.allocateMemory(textMemAlloc);
    device.bindBufferMemory(textBuffer, textMemory, 0);

    // Allocate and bind memory for pattern buffer.
    vk::MemoryAllocateInfo patternMemAlloc(patternMemReq.size, memoryTypeIndex);
    vk::DeviceMemory patternMemory = device.allocateMemory(patternMemAlloc);
    device.bindBufferMemory(patternBuffer, patternMemory, 0);

    // Allocate and bind memory for results buffer.
    vk::MemoryAllocateInfo resultsMemAlloc(resultsMemReq.size, memoryTypeIndex);
    vk::DeviceMemory resultsMemory = device.allocateMemory(resultsMemAlloc);
    device.bindBufferMemory(resultsBuffer, resultsMemory, 0);

    // Copy text into the text buffer.
    {
        void* textPtr = device.mapMemory(textMemory, 0, textBufferSize);
        memcpy(textPtr, text.data(), text.size());
        device.unmapMemory(textMemory);
    }

    // Copy pattern into the pattern buffer.
    {
        void* patternPtr = device.mapMemory(patternMemory, 0, patternBufferSize);
        memcpy(patternPtr, pattern.data(), pattern.size());
        device.unmapMemory(patternMemory);
    }

    // Initialize the results buffer:
    // * Set results[0] to 0 (atomic counter).
    // * Set all remaining indices to -1.
    {
        int32_t* resultsPtr = static_cast<int32_t*>(device.mapMemory(resultsMemory, 0, resultsBufferSize));
        resultsPtr[0] = 0;
        for (uint32_t i = 1; i < maxOccurrences; i++) {
            resultsPtr[i] = -1;
        }
        device.unmapMemory(resultsMemory);
    }

    ////////////////////////////////////////////////////////////////////////
    //                              PIPELINE                              //
    ////////////////////////////////////////////////////////////////////////
    // Load the compute shader (compiled to SPIR-V).
    std::vector<char> shaderContents = LoadShaderFile("../substringSearch.spv");
    if (shaderContents.empty())
        throw std::runtime_error("Failed to load substringSearch.spv");

    vk::ShaderModuleCreateInfo shaderModuleCreateInfo(
        vk::ShaderModuleCreateFlags(),
        shaderContents.size(),
        reinterpret_cast<const uint32_t*>(shaderContents.data())
    );
    vk::ShaderModule shaderModule = device.createShaderModule(shaderModuleCreateInfo);

    // Create descriptor set layout.
    // Binding 0: Text buffer (read-only)
    // Binding 1: Pattern buffer (read-only)
    // Binding 2: Results buffer (read/write)
    std::vector<vk::DescriptorSetLayoutBinding> layoutBindings = {
        { 0, vk::DescriptorType::eStorageBuffer, 1, vk::ShaderStageFlagBits::eCompute },
        { 1, vk::DescriptorType::eStorageBuffer, 1, vk::ShaderStageFlagBits::eCompute },
        { 2, vk::DescriptorType::eStorageBuffer, 1, vk::ShaderStageFlagBits::eCompute }
    };
    vk::DescriptorSetLayoutCreateInfo descLayoutCreateInfo(
        vk::DescriptorSetLayoutCreateFlags(),
        static_cast<uint32_t>(layoutBindings.size()),
        layoutBindings.data()
    );
    vk::DescriptorSetLayout descriptorSetLayout = device.createDescriptorSetLayout(descLayoutCreateInfo);

    // Create a push constant range for the compute stage.
    vk::PushConstantRange pushConstantRange = {};
    pushConstantRange.stageFlags = vk::ShaderStageFlagBits::eCompute;
    pushConstantRange.offset = 0;
    pushConstantRange.size = sizeof(PushConsts);

    // Create pipeline layout (descriptor set layout + push constant range).
    vk::PipelineLayoutCreateInfo pipelineLayoutCreateInfo;
    pipelineLayoutCreateInfo.setLayoutCount = 1;
    pipelineLayoutCreateInfo.pSetLayouts = &descriptorSetLayout;
    pipelineLayoutCreateInfo.pushConstantRangeCount = 1;
    pipelineLayoutCreateInfo.pPushConstantRanges = &pushConstantRange;
    vk::PipelineLayout pipelineLayout = device.createPipelineLayout(pipelineLayoutCreateInfo);

    // Create a pipeline cache (optional).
    vk::PipelineCache pipelineCache = device.createPipelineCache(vk::PipelineCacheCreateInfo());

    // Create compute pipeline.
    vk::PipelineShaderStageCreateInfo pipelineShaderStageCreateInfo(
        vk::PipelineShaderStageCreateFlags(),
        vk::ShaderStageFlagBits::eCompute,
        shaderModule,
        "main"
    );
    vk::ComputePipelineCreateInfo computePipelineCreateInfo(
        vk::PipelineCreateFlags(),
        pipelineShaderStageCreateInfo,
        pipelineLayout
    );
    vk::Pipeline computePipeline = device.createComputePipeline(pipelineCache, computePipelineCreateInfo).value;

    ////////////////////////////////////////////////////////////////////////
    //                          DESCRIPTOR SETS                           //
    ////////////////////////////////////////////////////////////////////////
    // Create a descriptor pool.
    vk::DescriptorPoolSize poolSize(vk::DescriptorType::eStorageBuffer, 3);
    vk::DescriptorPoolCreateInfo descriptorPoolCreateInfo(
        vk::DescriptorPoolCreateFlags(),
        1,         // Maximum number of sets
        1,         // Pool count for the given type
        &poolSize
    );
    vk::DescriptorPool descriptorPool = device.createDescriptorPool(descriptorPoolCreateInfo);

    // Allocate one descriptor set.
    vk::DescriptorSetAllocateInfo descriptorSetAllocInfo(
        descriptorPool,
        1,
        &descriptorSetLayout
    );
    std::vector<vk::DescriptorSet> descriptorSets = device.allocateDescriptorSets(descriptorSetAllocInfo);
    vk::DescriptorSet descriptorSet = descriptorSets.front();

    // Define descriptor buffer infos.
    vk::DescriptorBufferInfo textBufferInfo(textBuffer, 0, textBufferSize);
    vk::DescriptorBufferInfo patternBufferInfo(patternBuffer, 0, patternBufferSize);
    vk::DescriptorBufferInfo resultsBufferInfo(resultsBuffer, 0, resultsBufferSize);

    // Update the descriptor set.
    std::vector<vk::WriteDescriptorSet> writeSets = {
        { descriptorSet, 0, 0, 1, vk::DescriptorType::eStorageBuffer, nullptr, &textBufferInfo },
        { descriptorSet, 1, 0, 1, vk::DescriptorType::eStorageBuffer, nullptr, &patternBufferInfo },
        { descriptorSet, 2, 0, 1, vk::DescriptorType::eStorageBuffer, nullptr, &resultsBufferInfo }
    };
    device.updateDescriptorSets(writeSets, {});

    ////////////////////////////////////////////////////////////////////////
    //                         SUBMIT WORK TO GPU                         //
    ////////////////////////////////////////////////////////////////////////
    // Create a command pool.
    vk::CommandPoolCreateInfo commandPoolCreateInfo(vk::CommandPoolCreateFlags(), computeQueueFamilyIndex);
    vk::CommandPool commandPool = device.createCommandPool(commandPoolCreateInfo);

    // Allocate a command buffer.
    vk::CommandBufferAllocateInfo commandBufferAllocInfo(
        commandPool,
        vk::CommandBufferLevel::ePrimary,
        1
    );
    std::vector<vk::CommandBuffer> commandBuffers = device.allocateCommandBuffers(commandBufferAllocInfo);
    vk::CommandBuffer cmdBuffer = commandBuffers.front();

    // Record commands.
    vk::CommandBufferBeginInfo beginInfoCmd(vk::CommandBufferUsageFlagBits::eOneTimeSubmit);
    cmdBuffer.begin(beginInfoCmd);

    // Bind pipeline and descriptor sets.
    cmdBuffer.bindPipeline(vk::PipelineBindPoint::eCompute, computePipeline);
    cmdBuffer.bindDescriptorSets(
        vk::PipelineBindPoint::eCompute,
        pipelineLayout,
        0,
        { descriptorSet },
        {}
    );

    // Prepare push constants.
    PushConsts pushData{};
    pushData.textLength = static_cast<uint32_t>(text.size());
    pushData.patternLength = static_cast<uint32_t>(pattern.size());
    cmdBuffer.pushConstants<PushConsts>(
        pipelineLayout,
        vk::ShaderStageFlagBits::eCompute,
        0,
        pushData
    );

    // Dispatch: one thread per character (naive approach).
    uint32_t dispatchSize = static_cast<uint32_t>(text.size());
    cmdBuffer.dispatch(dispatchSize, 1, 1);
    cmdBuffer.end();

    // Submit command buffer.
    vk::Queue queue = device.getQueue(computeQueueFamilyIndex, 0);
    vk::Fence fence = device.createFence(vk::FenceCreateInfo());
    vk::SubmitInfo submitInfo(
        0, nullptr, nullptr,
        1, &cmdBuffer
    );
    queue.submit({ submitInfo }, fence);
    device.waitForFences({ fence }, true, uint64_t(-1));

    ////////////////////////////////////////////////////////////////////////
    //                        READ BACK THE RESULTS                       //
    ////////////////////////////////////////////////////////////////////////
    {
        int32_t* resultsPtr = static_cast<int32_t*>(device.mapMemory(resultsMemory, 0, resultsBufferSize));
        // The first element holds the number of matches found.
        uint32_t count = resultsPtr[0];
        std::cout << "Text size: " << text.size() << std::endl;
        std::cout << "Pattern: \"" << pattern << "\"" << std::endl;
        std::cout << "Matches (" << count << " occurrences):" << std::endl;
        for (uint32_t i = 0; i < count && i < (maxOccurrences - 1); i++) {
            std::cout << resultsPtr[1 + i] << " ";
        }
        std::cout << std::endl;
        device.unmapMemory(resultsMemory);
    }

    ////////////////////////////////////////////////////////////////////////
    //                              CLEANUP                               //
    ////////////////////////////////////////////////////////////////////////
    device.destroyFence(fence);
    device.destroyPipeline(computePipeline);
    device.destroyShaderModule(shaderModule);
    device.destroyPipelineCache(pipelineCache);
    device.destroyDescriptorPool(descriptorPool);
    device.destroyDescriptorSetLayout(descriptorSetLayout);
    device.destroyPipelineLayout(pipelineLayout);
    device.destroyCommandPool(commandPool);
    device.freeMemory(textMemory);
    device.freeMemory(patternMemory);
    device.freeMemory(resultsMemory);
    device.destroyBuffer(textBuffer);
    device.destroyBuffer(patternBuffer);
    device.destroyBuffer(resultsBuffer);
    device.destroy();
    instance.destroy();
}

void vulkanSquare() {
////////////////////////////////////////////////////////////////////////
//                          VULKAN INSTANCE                           //
////////////////////////////////////////////////////////////////////////
	vk::ApplicationInfo AppInfo{
		"VulkanCompute",      // Application Name
		1,                    // Application Version
		nullptr,              // Engine Name or nullptr
		0,                    // Engine Version
		VK_API_VERSION_1_1    // Vulkan API version
	};

    // Define validation layers (only)
    const std::vector<const char*> Layers = { 
        "VK_LAYER_KHRONOS_validation" 
    };

    // Define instance extensions (include the portability extension here)
    const std::vector<const char*> Extensions = {
        "VK_KHR_portability_enumeration",
        // Add other necessary extensions here (e.g., surface extensions) if needed.
    };

    // Create instance with flags, layers, and extensions
    vk::InstanceCreateInfo InstanceCreateInfo (
        vk::InstanceCreateFlags(vk::InstanceCreateFlagBits::eEnumeratePortabilityKHR), // Flags
        &AppInfo,                            // Application info
        Layers.size(),                       // Layers count
        Layers.data(),                       // Layers
        Extensions.size(),                   // Extensions count
        Extensions.data()                    // Extensions
    );

    vk::Instance Instance = vk::createInstance(InstanceCreateInfo);


////////////////////////////////////////////////////////////////////////
//                          PHYSICAL DEVICE                           //
////////////////////////////////////////////////////////////////////////
	vk::PhysicalDevice PhysicalDevice = Instance.enumeratePhysicalDevices().front();
	vk::PhysicalDeviceProperties DeviceProps = PhysicalDevice.getProperties();
	std::cout << "Device Name    : " << DeviceProps.deviceName << std::endl;
	const uint32_t ApiVersion = DeviceProps.apiVersion;
	std::cout << "Vulkan Version : " << VK_VERSION_MAJOR(ApiVersion) << "." << VK_VERSION_MINOR(ApiVersion) << "." << VK_VERSION_PATCH(ApiVersion) << std::endl;
	// vk::PhysicalDeviceLimits DeviceLimits = DeviceProps.limits;
	// std::cout << "Max Compute Shared Memory Size: " << DeviceLimits.maxComputeSharedMemorySize / 1024 << " KB" << std::endl;


////////////////////////////////////////////////////////////////////////
//                            QUEUE FAMILY                            //
////////////////////////////////////////////////////////////////////////
	std::vector<vk::QueueFamilyProperties> QueueFamilyProps = PhysicalDevice.getQueueFamilyProperties();
	auto PropIt = std::find_if(QueueFamilyProps.begin(), QueueFamilyProps.end(), [](const vk::QueueFamilyProperties& Prop) {
		return Prop.queueFlags & vk::QueueFlagBits::eCompute;
	});
	const uint32_t ComputeQueueFamilyIndex = std::distance(QueueFamilyProps.begin(), PropIt);
	std::cout << "Compute Queue Family Index: " << ComputeQueueFamilyIndex << std::endl;


////////////////////////////////////////////////////////////////////////
//                               DEVICE                               //
////////////////////////////////////////////////////////////////////////
	float queuePriorities = 1.0f;
	vk::DeviceQueueCreateInfo DeviceQueueCreateInfo(
			vk::DeviceQueueCreateFlags(),   // Flags
			ComputeQueueFamilyIndex,        // Queue Family Index
			1,                              // Number of Queues
			&queuePriorities
			);

    // Device creation extensions
    const std::vector<const char*> deviceExtensions = {
        "VK_KHR_portability_subset"
        // Add other device extensions if needed.
    };

    vk::DeviceCreateInfo DeviceCreateInfo(
        vk::DeviceCreateFlags(),
        1,
        &DeviceQueueCreateInfo,
        0,
        nullptr, // or a valid features pointer if needed
        deviceExtensions.size(),
        deviceExtensions.data()
    );
    vk::Device Device = PhysicalDevice.createDevice(DeviceCreateInfo);

////////////////////////////////////////////////////////////////////////
//                         Allocating Memory                          //
////////////////////////////////////////////////////////////////////////
	// Create the required buffers for the application
    // Allocate the memory to back the buffers
    // Bind the buffers to the memory
	
	// Create buffers
	const uint32_t NumElements = 10;
	const uint32_t BufferSize = NumElements * sizeof(int32_t);

	vk::BufferCreateInfo BufferCreateInfo{
		vk::BufferCreateFlags(),                    // Flags
		BufferSize,                                 // Size
		vk::BufferUsageFlagBits::eStorageBuffer,    // Usage
		vk::SharingMode::eExclusive,                // Sharing mode
		1,                                          // Number of queue family indices
		&ComputeQueueFamilyIndex                    // List of queue family indices
	};
	vk::Buffer InBuffer = Device.createBuffer(BufferCreateInfo);
	vk::Buffer OutBuffer = Device.createBuffer(BufferCreateInfo);

	// Memory req
	vk::MemoryRequirements InBufferMemoryRequirements = Device.getBufferMemoryRequirements(InBuffer);
	vk::MemoryRequirements OutBufferMemoryRequirements = Device.getBufferMemoryRequirements(OutBuffer);

	// query
	vk::PhysicalDeviceMemoryProperties MemoryProperties = PhysicalDevice.getMemoryProperties();

	uint32_t MemoryTypeIndex = uint32_t(~0);
	vk::DeviceSize MemoryHeapSize = uint32_t(~0);
	for (uint32_t CurrentMemoryTypeIndex = 0; CurrentMemoryTypeIndex < MemoryProperties.memoryTypeCount; ++CurrentMemoryTypeIndex)
	{
		vk::MemoryType MemoryType = MemoryProperties.memoryTypes[CurrentMemoryTypeIndex];
		if ((vk::MemoryPropertyFlagBits::eHostVisible & MemoryType.propertyFlags) &&
			(vk::MemoryPropertyFlagBits::eHostCoherent & MemoryType.propertyFlags))
		{
			MemoryHeapSize = MemoryProperties.memoryHeaps[MemoryType.heapIndex].size;
			MemoryTypeIndex = CurrentMemoryTypeIndex;
			break;
		}
	}

	std::cout << "Memory Type Index: " << MemoryTypeIndex << std::endl;
	std::cout << "Memory Heap Size : " << MemoryHeapSize / 1024 / 1024 / 1024 << " GB" << std::endl;

	// Allocate memory
	vk::MemoryAllocateInfo InBufferMemoryAllocateInfo(InBufferMemoryRequirements.size, MemoryTypeIndex);
	vk::MemoryAllocateInfo OutBufferMemoryAllocateInfo(OutBufferMemoryRequirements.size, MemoryTypeIndex);
	vk::DeviceMemory InBufferMemory = Device.allocateMemory(InBufferMemoryAllocateInfo);
	vk::DeviceMemory OutBufferMemory = Device.allocateMemory(InBufferMemoryAllocateInfo);

	// Map memory and write
	int32_t* InBufferPtr = static_cast<int32_t*>(Device.mapMemory(InBufferMemory, 0, BufferSize));
	for (uint32_t I = 0; I < NumElements; ++I) {
		InBufferPtr[I] = I;
	}
	Device.unmapMemory(InBufferMemory);

	// Bind buffers to memory
	Device.bindBufferMemory(InBuffer, InBufferMemory, 0);
	Device.bindBufferMemory(OutBuffer, OutBufferMemory, 0);

////////////////////////////////////////////////////////////////////////
//                              PIPELINE                              //
////////////////////////////////////////////////////////////////////////

    //ShaderContents contains the data in .spv shader file
	std::vector<char> ShaderContents;
	if (std::ifstream ShaderFile{ "../shader.comp.spv", std::ios::binary | std::ios::ate }) {
		const size_t FileSize = ShaderFile.tellg();
		ShaderFile.seekg(0);
		ShaderContents.resize(FileSize, '\0');
		ShaderFile.read(ShaderContents.data(), FileSize);
	}

  	vk::ShaderModuleCreateInfo ShaderModuleCreateInfo(
		vk::ShaderModuleCreateFlags(),                                // Flags
		ShaderContents.size(),                                        // Code size
		reinterpret_cast<const uint32_t*>(ShaderContents.data()));    // Code
    vk::ShaderModule ShaderModule = Device.createShaderModule(ShaderModuleCreateInfo);

	// Descriptor Set Layout
	// The layout of data to be passed to pipelin
	const std::vector<vk::DescriptorSetLayoutBinding> DescriptorSetLayoutBinding = {
		{0, vk::DescriptorType::eStorageBuffer, 1, vk::ShaderStageFlagBits::eCompute},
		{1, vk::DescriptorType::eStorageBuffer, 1, vk::ShaderStageFlagBits::eCompute}
	};
	vk::DescriptorSetLayoutCreateInfo DescriptorSetLayoutCreateInfo(
		vk::DescriptorSetLayoutCreateFlags(),
		DescriptorSetLayoutBinding);
	vk::DescriptorSetLayout DescriptorSetLayout = Device.createDescriptorSetLayout(DescriptorSetLayoutCreateInfo);

	// Pipeline Layout
	vk::PipelineLayoutCreateInfo PipelineLayoutCreateInfo(vk::PipelineLayoutCreateFlags(), DescriptorSetLayout);
	vk::PipelineLayout PipelineLayout = Device.createPipelineLayout(PipelineLayoutCreateInfo);
	vk::PipelineCache PipelineCache = Device.createPipelineCache(vk::PipelineCacheCreateInfo());

	// Compute Pipeline
	vk::PipelineShaderStageCreateInfo PipelineShaderCreateInfo(
		vk::PipelineShaderStageCreateFlags(),  // Flags
		vk::ShaderStageFlagBits::eCompute,     // Stage
		ShaderModule,                          // Shader Module
		"main"                                 // Shader Entry Point
		);
	vk::ComputePipelineCreateInfo ComputePipelineCreateInfo(
		vk::PipelineCreateFlags(),    // Flags
		PipelineShaderCreateInfo,     // Shader Create Info struct
		PipelineLayout                // Pipeline Layout
		);
	vk::Pipeline ComputePipeline = Device.createComputePipeline(PipelineCache, ComputePipelineCreateInfo).value;

////////////////////////////////////////////////////////////////////////
//                          DESCRIPTOR SETS                           //
////////////////////////////////////////////////////////////////////////
	// Descriptor sets must be allocated in a vk::DescriptorPool, so we need to create one first
	vk::DescriptorPoolSize DescriptorPoolSize(vk::DescriptorType::eStorageBuffer, 2);
	vk::DescriptorPoolCreateInfo DescriptorPoolCreateInfo(vk::DescriptorPoolCreateFlags(), 1, DescriptorPoolSize);
	vk::DescriptorPool DescriptorPool = Device.createDescriptorPool(DescriptorPoolCreateInfo);

	// Allocate descriptor sets, update them to use buffers:
	vk::DescriptorSetAllocateInfo DescriptorSetAllocInfo(DescriptorPool, 1, &DescriptorSetLayout);
	const std::vector<vk::DescriptorSet> DescriptorSets = Device.allocateDescriptorSets(DescriptorSetAllocInfo);
	vk::DescriptorSet DescriptorSet = DescriptorSets.front();
	vk::DescriptorBufferInfo InBufferInfo(InBuffer, 0, NumElements * sizeof(int32_t));
	vk::DescriptorBufferInfo OutBufferInfo(OutBuffer, 0, NumElements * sizeof(int32_t));

	const std::vector<vk::WriteDescriptorSet> WriteDescriptorSets = {
		{DescriptorSet, 0, 0, 1, vk::DescriptorType::eStorageBuffer, nullptr, &InBufferInfo},
		{DescriptorSet, 1, 0, 1, vk::DescriptorType::eStorageBuffer, nullptr, &OutBufferInfo},
	};
	Device.updateDescriptorSets(WriteDescriptorSets, {});

////////////////////////////////////////////////////////////////////////
//                         SUBMIT WORK TO GPU                         //
////////////////////////////////////////////////////////////////////////
    ////////////////////////////////////////////////////////////////////////
	// Command Pool
	vk::CommandPoolCreateInfo CommandPoolCreateInfo(vk::CommandPoolCreateFlags(), ComputeQueueFamilyIndex);
	vk::CommandPool CommandPool = Device.createCommandPool(CommandPoolCreateInfo);
	// Allocate Command buffer from Pool
	vk::CommandBufferAllocateInfo CommandBufferAllocInfo(
    CommandPool,                         // Command Pool
    vk::CommandBufferLevel::ePrimary,    // Level
    1);                                  // Num Command Buffers
	const std::vector<vk::CommandBuffer> CmdBuffers = Device.allocateCommandBuffers(CommandBufferAllocInfo);
	vk::CommandBuffer CmdBuffer = CmdBuffers.front();

	// Record commands
	vk::CommandBufferBeginInfo CmdBufferBeginInfo(vk::CommandBufferUsageFlagBits::eOneTimeSubmit);
	CmdBuffer.begin(CmdBufferBeginInfo);
	CmdBuffer.bindPipeline(vk::PipelineBindPoint::eCompute, ComputePipeline);
	CmdBuffer.bindDescriptorSets(
			vk::PipelineBindPoint::eCompute,    // Bind point
			PipelineLayout,                  // Pipeline Layout
			0,                               // First descriptor set
			{ DescriptorSet },               // List of descriptor sets
			{});                             // Dynamic offsets
	CmdBuffer.dispatch(NumElements, 1, 1);
	CmdBuffer.end();

	// Fence and submit
	vk::Queue Queue = Device.getQueue(ComputeQueueFamilyIndex, 0);
	vk::Fence Fence = Device.createFence(vk::FenceCreateInfo());
	vk::SubmitInfo SubmitInfo(
			0,                // Num Wait Semaphores
			nullptr,        // Wait Semaphores
			nullptr,        // Pipeline Stage Flags
			1,              // Num Command Buffers
			&CmdBuffer);    // List of command buffers
	Queue.submit({ SubmitInfo }, Fence);
	(void) Device.waitForFences(
			{ Fence },             // List of fences
			true,               // Wait All
			uint64_t(-1));      // Timeout

	// Map output buffer and read results
	InBufferPtr = static_cast<int32_t*>(Device.mapMemory(InBufferMemory, 0, BufferSize));
	std::cout << std::endl;
	std::cout << "INPUT:  ";
	for (uint32_t I = 0; I < NumElements; ++I) {
		std::cout << InBufferPtr[I] << " ";
	}
	std::cout << std::endl;
	Device.unmapMemory(InBufferMemory);

	int32_t* OutBufferPtr = static_cast<int32_t*>(Device.mapMemory(OutBufferMemory, 0, BufferSize));
	std::cout << "OUTPUT: ";
	for (uint32_t I = 0; I < NumElements; ++I) {
		std::cout << OutBufferPtr[I] << " ";
	}
	std::cout << std::endl;
	Device.unmapMemory(OutBufferMemory);
////////////////////////////////////////////////////////////////////////
//                              CLEANUP                               //
////////////////////////////////////////////////////////////////////////
	Device.resetCommandPool(CommandPool, vk::CommandPoolResetFlags());
	Device.destroyFence(Fence);
	Device.destroyDescriptorSetLayout(DescriptorSetLayout);
	Device.destroyPipelineLayout(PipelineLayout);
	Device.destroyPipelineCache(PipelineCache);
	Device.destroyShaderModule(ShaderModule);
	Device.destroyPipeline(ComputePipeline);
	Device.destroyDescriptorPool(DescriptorPool);
	Device.destroyCommandPool(CommandPool);
	Device.freeMemory(InBufferMemory);
	Device.freeMemory(OutBufferMemory);
	Device.destroyBuffer(InBuffer);
	Device.destroyBuffer(OutBuffer);
	Device.destroy();
	Instance.destroy();
}


