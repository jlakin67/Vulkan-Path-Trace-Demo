#pragma warning(disable : 26812)
#include "VulkanApp.h"

double currentTime = 0.0f, lastTime = 0.0f, deltaTime = 0.0f;
bool firstMove = true, mousePressed = false;
double lastX = 0, lastY = 0, deltaX = 0, deltaY = 0;
Camera camera;
void cursor_position_callback(GLFWwindow* window, double xpos, double ypos) {
    if (firstMove) {
        lastX = xpos;
        lastY = ypos;
        firstMove = false;
    }
    else {
        deltaX = xpos - lastX;
        deltaY = ypos - lastY;
    }
    if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_RELEASE) {
        deltaX = 0;
        deltaY = 0;
    }
    lastX = xpos; lastY = ypos;
    camera.processMouse(window, deltaX, deltaY);
}

void processInput(GLFWwindow* window, Camera& camera) {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) glfwSetWindowShouldClose(window, true);
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
    {
        camera.processKeyboard(GLFW_KEY_W, deltaTime);
    }
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
        camera.processKeyboard(GLFW_KEY_S, deltaTime);
    }
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
        camera.processKeyboard(GLFW_KEY_D, deltaTime);
    }
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
        camera.processKeyboard(GLFW_KEY_A, deltaTime);
    }
}

void VulkanApp::run() {
    setup = VulkanSetup::getInstance();
    initWindow();
    initVulkan();
    mainLoop();
    cleanup();
}

void VulkanApp::mainLoop() {
	while (!glfwWindowShouldClose(window)) {
		glfwPollEvents();
		currentTime = glfwGetTime();
		deltaTime = currentTime - lastTime;
        processInput(window, camera);
        updateUniformBuffer();
        renderFrame();
	}
    vkDeviceWaitIdle(logicalDevice);
}

void VulkanApp::loadProxyFuncs() {
    vkCmdBuildAccelerationStructuresKHR = reinterpret_cast<PFN_vkCmdBuildAccelerationStructuresKHR>(vkGetDeviceProcAddr(logicalDevice, "vkCmdBuildAccelerationStructuresKHR"));
    vkBuildAccelerationStructuresKHR = reinterpret_cast<PFN_vkBuildAccelerationStructuresKHR>(vkGetDeviceProcAddr(logicalDevice, "vkBuildAccelerationStructuresKHR"));
    vkCreateAccelerationStructureKHR = reinterpret_cast<PFN_vkCreateAccelerationStructureKHR>(vkGetDeviceProcAddr(logicalDevice, "vkCreateAccelerationStructureKHR"));
    vkDestroyAccelerationStructureKHR = reinterpret_cast<PFN_vkDestroyAccelerationStructureKHR>(vkGetDeviceProcAddr(logicalDevice, "vkDestroyAccelerationStructureKHR"));
    vkGetAccelerationStructureBuildSizesKHR = reinterpret_cast<PFN_vkGetAccelerationStructureBuildSizesKHR>(vkGetDeviceProcAddr(logicalDevice, "vkGetAccelerationStructureBuildSizesKHR"));
    vkGetAccelerationStructureDeviceAddressKHR = reinterpret_cast<PFN_vkGetAccelerationStructureDeviceAddressKHR>(vkGetDeviceProcAddr(logicalDevice, "vkGetAccelerationStructureDeviceAddressKHR"));
    vkCmdTraceRaysKHR = reinterpret_cast<PFN_vkCmdTraceRaysKHR>(vkGetDeviceProcAddr(logicalDevice, "vkCmdTraceRaysKHR"));
    vkGetRayTracingShaderGroupHandlesKHR = reinterpret_cast<PFN_vkGetRayTracingShaderGroupHandlesKHR>(vkGetDeviceProcAddr(logicalDevice, "vkGetRayTracingShaderGroupHandlesKHR"));
    vkCreateRayTracingPipelinesKHR = reinterpret_cast<PFN_vkCreateRayTracingPipelinesKHR>(vkGetDeviceProcAddr(logicalDevice, "vkCreateRayTracingPipelinesKHR"));
    if (!vkCmdBuildAccelerationStructuresKHR || !vkBuildAccelerationStructuresKHR 
        || !vkCreateAccelerationStructureKHR || !vkDestroyAccelerationStructureKHR
        || !vkGetAccelerationStructureBuildSizesKHR || !vkGetAccelerationStructureDeviceAddressKHR 
        || !vkCmdTraceRaysKHR || !vkGetRayTracingShaderGroupHandlesKHR
        || !vkCreateRayTracingPipelinesKHR)
        throw std::runtime_error("Failed to load proxy funcs!");
}

void VulkanApp::createBuffers() {
    VkDeviceSize indexBufferSize = sizeof(model.indices[0]) * model.indices.size();
    VkDeviceSize positionBufferSize = sizeof(model.positions[0]) * model.positions.size();
    VkDeviceSize stagingMemorySize = std::max(indexBufferSize, positionBufferSize);
    //VkDeviceSize colorBufferSize = sizeof(model.colors[0]) * model.colors.size();

    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;
    createBuffer(physicalDevice, logicalDevice, stagingMemorySize,
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        stagingBuffer, stagingBufferMemory);
    void* data;
    vkMapMemory(logicalDevice, stagingBufferMemory, 0, positionBufferSize, 0, &data);
    memcpy(data, model.positions.data(), (size_t)positionBufferSize);
    vkUnmapMemory(logicalDevice, stagingBufferMemory);
    createBuffer(physicalDevice, logicalDevice,
        positionBufferSize,
        VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR | 
        VK_BUFFER_USAGE_TRANSFER_DST_BIT | 
        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | 
        VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, positionBuffer, positionBufferMemory);
    copyBuffer(logicalDevice, graphicsQueue, commandPool, stagingBuffer, positionBuffer, positionBufferSize);
    
    vkMapMemory(logicalDevice, stagingBufferMemory, 0, indexBufferSize, 0, &data);
    memcpy(data, model.indices.data(), (size_t)indexBufferSize);
    vkUnmapMemory(logicalDevice, stagingBufferMemory);
    createBuffer(physicalDevice, logicalDevice,
        indexBufferSize,
        VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR |
        VK_BUFFER_USAGE_TRANSFER_DST_BIT |
        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT |
        VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, indexBuffer, indexBufferMemory);
    copyBuffer(logicalDevice, graphicsQueue, commandPool, stagingBuffer, indexBuffer, indexBufferSize);

    VkDeviceSize colorBufferSize = sizeof(model.colors[0]) * model.colors.size();
    vkMapMemory(logicalDevice, stagingBufferMemory, 0, colorBufferSize, 0, &data);
    memcpy(data, model.colors.data(), (size_t)colorBufferSize);
    vkUnmapMemory(logicalDevice, stagingBufferMemory);
    createBuffer(physicalDevice, logicalDevice,
        colorBufferSize,
        VK_BUFFER_USAGE_TRANSFER_DST_BIT |
        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT |
        VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, colorBuffer, colorBufferMemory);
    copyBuffer(logicalDevice, graphicsQueue, commandPool, stagingBuffer, colorBuffer, colorBufferSize);

    VkDeviceSize emissionBufferSize = sizeof(model.emissions[0]) * model.emissions.size();
    vkMapMemory(logicalDevice, stagingBufferMemory, 0, emissionBufferSize, 0, &data);
    memcpy(data, model.emissions.data(), (size_t)emissionBufferSize);
    vkUnmapMemory(logicalDevice, stagingBufferMemory);
    createBuffer(physicalDevice, logicalDevice,
        emissionBufferSize,
        VK_BUFFER_USAGE_TRANSFER_DST_BIT |
        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT |
        VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, emissionBuffer, emissionBufferMemory);
    copyBuffer(logicalDevice, graphicsQueue, commandPool, stagingBuffer, emissionBuffer, emissionBufferSize);

    vkDestroyBuffer(logicalDevice, stagingBuffer, nullptr);
    vkFreeMemory(logicalDevice, stagingBufferMemory, nullptr);

    VkBufferDeviceAddressInfo positionInfo{};
    positionInfo.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO;
    positionInfo.buffer = positionBuffer;
    positionBufferAddress = vkGetBufferDeviceAddress(logicalDevice, &positionInfo);

    VkBufferDeviceAddressInfo indexInfo{};
    indexInfo.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO;
    indexInfo.buffer = indexBuffer;
    indexBufferAddress = vkGetBufferDeviceAddress(logicalDevice, &indexInfo);
}

void VulkanApp::createAccelerationStructures() {
    VkAccelerationStructureGeometryTrianglesDataKHR triangles{
        VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_TRIANGLES_DATA_KHR };
    triangles.vertexFormat = VK_FORMAT_R32G32B32_SFLOAT;
    triangles.vertexData.deviceAddress = positionBufferAddress;
    triangles.vertexStride = 3 * sizeof(float);
    triangles.indexType = VK_INDEX_TYPE_UINT32;
    triangles.indexData.deviceAddress = indexBufferAddress;
    triangles.maxVertex = static_cast<uint32_t>(model.positions.size() - 1);
    triangles.transformData = { 0 };
    VkAccelerationStructureGeometryKHR geometry{
        VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR };
    geometry.geometryType = VK_GEOMETRY_TYPE_TRIANGLES_KHR;
    geometry.geometry.triangles = triangles;
    geometry.flags = VK_GEOMETRY_OPAQUE_BIT_KHR;
    VkAccelerationStructureBuildRangeInfoKHR rangeInfo{};
    rangeInfo.firstVertex = 0;
    rangeInfo.primitiveCount = static_cast<uint32_t>(model.indices.size() / 3);
    rangeInfo.primitiveOffset = 0;
    rangeInfo.transformOffset = 0;
    VkAccelerationStructureBuildGeometryInfoKHR buildInfo{ 
        VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR };
    buildInfo.flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;
    buildInfo.geometryCount = 1;
    buildInfo.pGeometries = &geometry;
    buildInfo.mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
    buildInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
    buildInfo.srcAccelerationStructure = VK_NULL_HANDLE;
    VkAccelerationStructureBuildSizesInfoKHR sizeInfo{
        VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR };
    vkGetAccelerationStructureBuildSizesKHR(
        // The device
        logicalDevice,
        // Build on device instead of host.
        VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR,
        // Pointer to build info
        &buildInfo,
        // Array of number of primitives per geometry
        &rangeInfo.primitiveCount,
        // Output pointer to store sizes
        &sizeInfo);

    createBuffer(physicalDevice, logicalDevice, sizeInfo.accelerationStructureSize,
        VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR
        | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT
        | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        blasBuffer, blasBufferMemory);
    VkAccelerationStructureCreateInfoKHR createInfo{
        VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR };
    createInfo.type = buildInfo.type;
    createInfo.size = sizeInfo.accelerationStructureSize;
    createInfo.buffer = blasBuffer;
    if (vkCreateAccelerationStructureKHR(logicalDevice, &createInfo, nullptr, &blas) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create bottom level acceleration structure!");
    }
    buildInfo.dstAccelerationStructure = blas;
    
    createBuffer(physicalDevice, logicalDevice, sizeInfo.buildScratchSize, VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT
        | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, blasScratchBuffer, blasScratchBufferMemory);
    VkBufferDeviceAddressInfo scratchInfo{};
    scratchInfo.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO;
    scratchInfo.buffer = blasScratchBuffer;
    buildInfo.scratchData.deviceAddress = vkGetBufferDeviceAddress(logicalDevice, &scratchInfo);
    VkCommandBuffer commandBuffer = beginSingleTimeCommands(logicalDevice, commandPool);
    VkAccelerationStructureBuildRangeInfoKHR* pRangeInfo = &rangeInfo;
    vkCmdBuildAccelerationStructuresKHR(commandBuffer, 1, &buildInfo, &pRangeInfo);
    endSingleTimeCommands(logicalDevice, graphicsQueue, commandPool, commandBuffer);
    VkAccelerationStructureDeviceAddressInfoKHR blasAddressInfo{
        VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_DEVICE_ADDRESS_INFO_KHR };
    blasAddressInfo.accelerationStructure = blas;
    VkDeviceAddress blasAddress = vkGetAccelerationStructureDeviceAddressKHR(logicalDevice, &blasAddressInfo);
    VkAccelerationStructureInstanceKHR instance{};
    instance.transform.matrix[0][0] = 1.0f;
    instance.transform.matrix[1][1] = 1.0f;
    instance.transform.matrix[2][2] = 1.0f;
    instance.instanceCustomIndex = 0;
    instance.mask = 0xFF;
    instance.instanceShaderBindingTableRecordOffset = 0;
    instance.flags = VK_GEOMETRY_INSTANCE_TRIANGLE_FACING_CULL_DISABLE_BIT_KHR;
    instance.accelerationStructureReference = blasAddress;
    createBuffer(physicalDevice, logicalDevice, sizeof(instance),
        VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT 
        | VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        instanceBuffer, instanceBufferMemory);
    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;
    createBuffer(physicalDevice, logicalDevice, sizeof(instance),
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        stagingBuffer, stagingBufferMemory);
    void* data;
    vkMapMemory(logicalDevice, stagingBufferMemory, 0, sizeof(instance), 0, &data);
    memcpy(data, &instance, sizeof(instance));
    vkUnmapMemory(logicalDevice, stagingBufferMemory);
    copyBuffer(logicalDevice, graphicsQueue, commandPool, stagingBuffer, instanceBuffer, sizeof(instance));
    vkDestroyBuffer(logicalDevice, stagingBuffer, nullptr);
    vkFreeMemory(logicalDevice, stagingBufferMemory, nullptr);
    VkAccelerationStructureBuildRangeInfoKHR tlasRangeInfo;
    tlasRangeInfo.primitiveOffset = 0;
    tlasRangeInfo.primitiveCount = 1;
    tlasRangeInfo.firstVertex = 0;
    tlasRangeInfo.transformOffset = 0;
    VkAccelerationStructureGeometryInstancesDataKHR instancesVk{
        VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_INSTANCES_DATA_KHR };
    instancesVk.arrayOfPointers = VK_FALSE;
    VkBufferDeviceAddressInfo instanceAddressInfo{};
    instanceAddressInfo.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO;
    instanceAddressInfo.buffer = instanceBuffer;
    VkDeviceAddress instanceAddress = vkGetBufferDeviceAddress(logicalDevice, &instanceAddressInfo);
    instancesVk.data.deviceAddress = instanceAddress;
    VkAccelerationStructureGeometryKHR tlasGeometry{ VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR };
    tlasGeometry.geometryType = VK_GEOMETRY_TYPE_INSTANCES_KHR;
    tlasGeometry.geometry.instances = instancesVk;
    VkAccelerationStructureBuildGeometryInfoKHR tlasBuildInfo{
        VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR };
    tlasBuildInfo.flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;
    tlasBuildInfo.geometryCount = 1;
    tlasBuildInfo.pGeometries = &tlasGeometry;
    tlasBuildInfo.mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
    tlasBuildInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;
    tlasBuildInfo.srcAccelerationStructure = VK_NULL_HANDLE;
    VkAccelerationStructureBuildSizesInfoKHR tlasSizeInfo{
        VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR };
    vkGetAccelerationStructureBuildSizesKHR(
        logicalDevice,
        VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR,
        &tlasBuildInfo,
        &tlasRangeInfo.primitiveCount,
        &tlasSizeInfo);
    createBuffer(physicalDevice, logicalDevice, tlasSizeInfo.accelerationStructureSize,
        VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR
        | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT
        | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, tlasBuffer, tlasBufferMemory);
    VkAccelerationStructureCreateInfoKHR tlasCreateInfo{
        VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR };
    tlasCreateInfo.type = tlasBuildInfo.type;
    tlasCreateInfo.size = tlasSizeInfo.accelerationStructureSize;
    tlasCreateInfo.buffer = tlasBuffer;
    tlasCreateInfo.offset = 0;
    if (vkCreateAccelerationStructureKHR(logicalDevice, &tlasCreateInfo, nullptr, &tlas) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create top level acceleration structure!");
    }
    tlasBuildInfo.dstAccelerationStructure = tlas;
    createBuffer(physicalDevice, logicalDevice, tlasSizeInfo.buildScratchSize, VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT
        | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, tlasScratchBuffer, tlasScratchBufferMemory);
    VkBufferDeviceAddressInfo tlasScratchAddressInfo{};
    tlasScratchAddressInfo.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO;
    tlasScratchAddressInfo.buffer = tlasScratchBuffer;
    tlasBuildInfo.scratchData.deviceAddress = vkGetBufferDeviceAddress(logicalDevice, &tlasScratchAddressInfo);
    VkAccelerationStructureBuildRangeInfoKHR* pTlasRangeInfo = &tlasRangeInfo;
    commandBuffer = beginSingleTimeCommands(logicalDevice, commandPool);
    vkCmdBuildAccelerationStructuresKHR(commandBuffer, 1, &tlasBuildInfo, &pTlasRangeInfo);
    endSingleTimeCommands(logicalDevice, graphicsQueue, commandPool, commandBuffer);
}

void VulkanApp::createUniformBuffer() {
    //only one frame in flight

    createBuffer(physicalDevice, logicalDevice, sizeof(scene), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | 
        VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, uniformBuffer, uniformBufferMemory);
}

void VulkanApp::updateUniformBuffer() {
    static bool firstUpdate = true;
    glm::mat4 view = glm::lookAt(camera.pos, camera.pos + camera.front, glm::vec3(0.0f, 0.0f, 1.0f));
    scene.viewInverse = glm::inverse(view);
    if (firstUpdate) {
        glm::mat4 projection = glm::perspective(glm::radians(ZOOM), swapchainExtent.width / (float)swapchainExtent.height, 0.1f, 10.0f);
        projection[1][1] *= -1;
        scene.projInverse = glm::inverse(projection);
        firstUpdate = false;
    }
    void* data;
    vkMapMemory(logicalDevice, uniformBufferMemory, 0, sizeof(scene), 0, &data);
    memcpy(data, &scene, sizeof(scene));
    vkUnmapMemory(logicalDevice, uniformBufferMemory);
}

void VulkanApp::createImageStorage() {
    createImage(physicalDevice, logicalDevice, swapchainExtent.width, swapchainExtent.height,
        1, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, storageImage, storageImageMemory);
    //transitionImageLayout(logicalDevice, graphicsQueue, commandPool, storageImage, VK_FORMAT_R8G8B8A8_UNORM,
     //   VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL, 1);
    storageImageView = createImageView(logicalDevice, storageImage, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_ASPECT_COLOR_BIT, 1);
    //VkSamplerCreateInfo samplerInfo{};
    //samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    //vkCreateSampler(logicalDevice, &samplerInfo, nullptr, &sampler);
}

void VulkanApp::createDescriptorSets() {
    std::array<VkDescriptorSetLayoutBinding, 7> bindings;
    bindings[0].binding = 0;
    bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR;
    bindings[0].descriptorCount = 1;
    bindings[0].stageFlags = VK_SHADER_STAGE_RAYGEN_BIT_KHR | VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR;
    bindings[1].binding = 1;
    bindings[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
    bindings[1].descriptorCount = 1;
    bindings[1].stageFlags = VK_SHADER_STAGE_RAYGEN_BIT_KHR;
    bindings[2].binding = 2;
    bindings[2].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    bindings[2].descriptorCount = 1;
    bindings[2].stageFlags = VK_SHADER_STAGE_RAYGEN_BIT_KHR | VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR;
    bindings[3].binding = 3;
    bindings[3].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    bindings[3].descriptorCount = 1;
    bindings[3].stageFlags = VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR;
    bindings[4] = bindings[3];
    bindings[4].binding = 4;
    bindings[5] = bindings[3];
    bindings[5].binding = 5;
    bindings[6] = bindings[3];
    bindings[6].binding = 6;
    VkDescriptorSetLayoutCreateInfo layoutInfo{
        VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO };
    layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
    layoutInfo.pBindings = bindings.data();
    if (vkCreateDescriptorSetLayout(logicalDevice, &layoutInfo, nullptr, &setLayout) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create descriptor layout!");
    }
    VkDescriptorPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.maxSets = 1;
    std::array<VkDescriptorPoolSize, 4> poolSizes;
    poolSizes[0].type = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR;
    poolSizes[0].descriptorCount = 1;
    poolSizes[1].type = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
    poolSizes[1].descriptorCount = 1;
    poolSizes[2].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    poolSizes[2].descriptorCount = 1;
    poolSizes[3].type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    poolSizes[3].descriptorCount = 4;
    poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
    poolInfo.pPoolSizes = poolSizes.data();
    if (vkCreateDescriptorPool(logicalDevice, &poolInfo, nullptr, &descriptorPool) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create descriptor pool!");
    }
    VkDescriptorSetAllocateInfo setInfo{};
    setInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    setInfo.descriptorPool = descriptorPool;
    setInfo.pSetLayouts = &setLayout;
    setInfo.descriptorSetCount = 1;
    if (vkAllocateDescriptorSets(logicalDevice, &setInfo, &descriptorSet) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create descriptor sets!");
    }

    VkWriteDescriptorSetAccelerationStructureKHR descriptorSetAccelerationStructure{};
    descriptorSetAccelerationStructure.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET_ACCELERATION_STRUCTURE_KHR;
    descriptorSetAccelerationStructure.accelerationStructureCount = 1;
    descriptorSetAccelerationStructure.pAccelerationStructures = &tlas;
    descriptorSetAccelerationStructure.pNext = nullptr;

    std::array<VkWriteDescriptorSet, 7> writeDescriptorSets{};
    writeDescriptorSets[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeDescriptorSets[0].descriptorType = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR;
    writeDescriptorSets[0].dstBinding = 0;
    writeDescriptorSets[0].dstArrayElement = 0;
    writeDescriptorSets[0].descriptorCount = 1;
    writeDescriptorSets[0].dstSet = descriptorSet;
    writeDescriptorSets[0].pNext = &descriptorSetAccelerationStructure;
    
    writeDescriptorSets[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeDescriptorSets[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
    writeDescriptorSets[1].dstBinding = 1;
    writeDescriptorSets[1].dstArrayElement = 0;
    writeDescriptorSets[1].descriptorCount = 1;
    writeDescriptorSets[1].dstSet = descriptorSet;
    VkDescriptorImageInfo imageInfo{};
    imageInfo.imageView = storageImageView;
    imageInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
    imageInfo.sampler = nullptr;
    writeDescriptorSets[1].pImageInfo = &imageInfo;
    writeDescriptorSets[1].pNext = nullptr;
    writeDescriptorSets[1].pBufferInfo = nullptr;
    writeDescriptorSets[1].pTexelBufferView = nullptr;
    
    writeDescriptorSets[2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeDescriptorSets[2].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    writeDescriptorSets[2].dstBinding = 2;
    writeDescriptorSets[2].dstArrayElement = 0;
    writeDescriptorSets[2].descriptorCount = 1;
    writeDescriptorSets[2].dstSet = descriptorSet;
    VkDescriptorBufferInfo uniformBufferInfo{};
    uniformBufferInfo.offset = 0;
    uniformBufferInfo.range = VK_WHOLE_SIZE;
    uniformBufferInfo.buffer = uniformBuffer;
    writeDescriptorSets[2].pBufferInfo = &uniformBufferInfo;
    writeDescriptorSets[2].pImageInfo = nullptr;
    writeDescriptorSets[2].pTexelBufferView = nullptr;
    writeDescriptorSets[2].pNext = nullptr;
    
    writeDescriptorSets[3].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeDescriptorSets[3].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writeDescriptorSets[3].dstBinding = 3;
    writeDescriptorSets[3].dstArrayElement = 0;
    writeDescriptorSets[3].descriptorCount = 1;
    writeDescriptorSets[3].dstSet = descriptorSet;
    writeDescriptorSets[3].pNext = nullptr;
    VkDescriptorBufferInfo indexBufferInfo{};
    indexBufferInfo.buffer = indexBuffer;
    indexBufferInfo.offset = 0;
    indexBufferInfo.range = VK_WHOLE_SIZE;
    writeDescriptorSets[3].pBufferInfo = &indexBufferInfo;
    writeDescriptorSets[3].pImageInfo = nullptr;
    writeDescriptorSets[3].pTexelBufferView = nullptr;
    
    writeDescriptorSets[4].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeDescriptorSets[4].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writeDescriptorSets[4].dstBinding = 4;
    writeDescriptorSets[4].dstArrayElement = 0;
    writeDescriptorSets[4].descriptorCount = 1;
    writeDescriptorSets[4].dstSet = descriptorSet;
    VkDescriptorBufferInfo positionBufferInfo{};
    positionBufferInfo.buffer = positionBuffer;
    positionBufferInfo.offset = 0;
    positionBufferInfo.range = VK_WHOLE_SIZE;
    writeDescriptorSets[4].pBufferInfo = &positionBufferInfo;
    writeDescriptorSets[4].pImageInfo = nullptr;
    writeDescriptorSets[4].pTexelBufferView = nullptr;
    writeDescriptorSets[4].pNext = nullptr;

    writeDescriptorSets[5].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeDescriptorSets[5].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writeDescriptorSets[5].dstBinding = 5;
    writeDescriptorSets[5].dstArrayElement = 0;
    writeDescriptorSets[5].descriptorCount = 1;
    writeDescriptorSets[5].dstSet = descriptorSet;
    VkDescriptorBufferInfo emissionBufferInfo{};
    emissionBufferInfo.buffer = emissionBuffer;
    emissionBufferInfo.offset = 0;
    emissionBufferInfo.range = VK_WHOLE_SIZE;
    writeDescriptorSets[5].pBufferInfo = &emissionBufferInfo;
    writeDescriptorSets[5].pImageInfo = nullptr;
    writeDescriptorSets[5].pTexelBufferView = nullptr;
    writeDescriptorSets[5].pNext = nullptr;

    writeDescriptorSets[6].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeDescriptorSets[6].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writeDescriptorSets[6].dstBinding = 6;
    writeDescriptorSets[6].dstArrayElement = 0;
    writeDescriptorSets[6].descriptorCount = 1;
    writeDescriptorSets[6].dstSet = descriptorSet;
    VkDescriptorBufferInfo colorBufferInfo{};
    colorBufferInfo.buffer = colorBuffer;
    colorBufferInfo.offset = 0;
    colorBufferInfo.range = VK_WHOLE_SIZE;
    writeDescriptorSets[6].pBufferInfo = &colorBufferInfo;
    writeDescriptorSets[6].pImageInfo = nullptr;
    writeDescriptorSets[6].pTexelBufferView = nullptr;
    writeDescriptorSets[6].pNext = nullptr;
    
    vkUpdateDescriptorSets(logicalDevice, static_cast<uint32_t>(writeDescriptorSets.size()), writeDescriptorSets.data(), 0, nullptr);
}

void VulkanApp::createRaytracingPipeline() {
    std::vector<char> rgenSource = readSourceFile("Shaders/raygen.rgen.spv");
    VkShaderModule rgen = createShaderModule(logicalDevice, rgenSource);
    std::vector<char> rchitSource = readSourceFile("Shaders/closesthit.rchit.spv");
    VkShaderModule rchit = createShaderModule(logicalDevice, rchitSource);
    std::vector<char> rmissSource = readSourceFile("Shaders/miss.rmiss.spv");
    VkShaderModule rmiss = createShaderModule(logicalDevice, rmissSource);
    std::array<VkPipelineShaderStageCreateInfo, 3> pssci{};
    pssci[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    pssci[0].module = rgen;
    pssci[0].pName = "main";
    pssci[0].stage = VK_SHADER_STAGE_RAYGEN_BIT_KHR;
    pssci[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    pssci[1].module = rmiss;
    pssci[1].pName = "main";
    pssci[1].stage = VK_SHADER_STAGE_MISS_BIT_KHR;
    pssci[2].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    pssci[2].module = rchit;
    pssci[2].pName = "main";
    pssci[2].stage = VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR;
    std::array<VkRayTracingShaderGroupCreateInfoKHR, 3> rtsgci{};
    rtsgci[0].sType = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR;
    rtsgci[0].type = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR;
    rtsgci[0].generalShader = 0;
    rtsgci[0].closestHitShader = VK_SHADER_UNUSED_KHR;
    rtsgci[0].anyHitShader = VK_SHADER_UNUSED_KHR;
    rtsgci[0].intersectionShader = VK_SHADER_UNUSED_KHR;
    rtsgci[1] = rtsgci[0];
    rtsgci[1].generalShader = 1;
    rtsgci[2].sType = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR;
    rtsgci[2].type =  VK_RAY_TRACING_SHADER_GROUP_TYPE_TRIANGLES_HIT_GROUP_KHR;
    rtsgci[2].generalShader = VK_SHADER_UNUSED_KHR;
    rtsgci[2].closestHitShader = 2;
    rtsgci[2].anyHitShader = VK_SHADER_UNUSED_KHR;
    rtsgci[2].intersectionShader = VK_SHADER_UNUSED_KHR;
    VkRayTracingPipelineCreateInfoKHR rtpci{VK_STRUCTURE_TYPE_RAY_TRACING_PIPELINE_CREATE_INFO_KHR };
    rtpci.stageCount = static_cast<uint32_t>(pssci.size());
    rtpci.pStages = pssci.data();
    rtpci.groupCount = static_cast<uint32_t>(rtsgci.size());
    rtpci.pGroups = rtsgci.data();
    rtpci.maxPipelineRayRecursionDepth = 3;
    VkPipelineLayoutCreateInfo layoutInfo{};
    layoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    layoutInfo.setLayoutCount = 1;
    layoutInfo.pSetLayouts = &setLayout;
    vkCreatePipelineLayout(logicalDevice, &layoutInfo, nullptr, &pipelineLayout);
    rtpci.layout = pipelineLayout;
    rtpci.pLibraryInfo = nullptr;
    rtpci.pLibraryInterface = nullptr;
    rtpci.basePipelineIndex = -1;
    if (vkCreateRayTracingPipelinesKHR(logicalDevice, VK_NULL_HANDLE, VK_NULL_HANDLE, 1, &rtpci, nullptr, &pipeline) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create ray tracing pipeline!");
    }
    vkDestroyShaderModule(logicalDevice, rgen, nullptr);
    vkDestroyShaderModule(logicalDevice, rmiss, nullptr);
    vkDestroyShaderModule(logicalDevice, rchit, nullptr);
    uint32_t groupCount = static_cast<uint32_t>(rtsgci.size());
    uint32_t groupHandleSize = rtProperties.shaderGroupHandleSize;
    uint32_t groupBaseAlignment = rtProperties.shaderGroupBaseAlignment;
    //https://github.com/nvpro-samples/nvpro_core/blob/master/nvh/alignment.hpp
    uint32_t groupSizeAligned = uint32_t((groupHandleSize + (uint32_t(groupBaseAlignment) - 1)) & ~uint32_t(groupBaseAlignment - 1));
    uint32_t sbtSize = groupCount * groupSizeAligned;
    std::vector<uint8_t> shaderHandleStorage(sbtSize);
    if (vkGetRayTracingShaderGroupHandlesKHR(logicalDevice, pipeline, 0, groupCount, sbtSize, shaderHandleStorage.data()) != VK_SUCCESS) {
        throw std::runtime_error("Failed to retrieve shader handles!");
    };
    createBuffer(physicalDevice, logicalDevice, sbtSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT
        | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT
        | VK_BUFFER_USAGE_SHADER_BINDING_TABLE_BIT_KHR, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT
        | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, shaderBindingTable, shaderBindingTableMemory);
    void* data;
    vkMapMemory(logicalDevice, shaderBindingTableMemory, 0, sbtSize, 0, &data);
    auto* pData = reinterpret_cast<uint8_t*>(data);
    for (uint32_t g = 0; g < groupCount; g++) {
        memcpy(pData, shaderHandleStorage.data() + g * groupHandleSize,
            groupHandleSize);
        pData += groupSizeAligned;
    }
    vkUnmapMemory(logicalDevice, shaderBindingTableMemory);

    commandBuffers.resize(swapchainImages.size());
    for (int i = 0; i < swapchainImages.size(); i++) {
        VkCommandBufferAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocInfo.commandPool = commandPool;
        allocInfo.commandBufferCount = 1;
        allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        if (vkAllocateCommandBuffers(logicalDevice, &allocInfo, &commandBuffers[i]) != VK_SUCCESS) {
            throw std::runtime_error("Failed to allocate command buffer!");
        }
        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
        if (vkBeginCommandBuffer(commandBuffers[i], &beginInfo) != VK_SUCCESS) {
            throw std::runtime_error("Failed to begin recording command buffer!");
        }

        VkImageMemoryBarrier initialRaytraceBarrier{};
        initialRaytraceBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        initialRaytraceBarrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        initialRaytraceBarrier.newLayout = VK_IMAGE_LAYOUT_GENERAL;
        initialRaytraceBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        initialRaytraceBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        initialRaytraceBarrier.image = storageImage;
        initialRaytraceBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        initialRaytraceBarrier.subresourceRange.baseMipLevel = 0;
        initialRaytraceBarrier.subresourceRange.levelCount = 1;
        initialRaytraceBarrier.subresourceRange.baseArrayLayer = 0;
        initialRaytraceBarrier.subresourceRange.layerCount = 1;
        initialRaytraceBarrier.srcAccessMask = 0;
        initialRaytraceBarrier.dstAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
        vkCmdPipelineBarrier(commandBuffers[i], VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_RAY_TRACING_SHADER_BIT_KHR, 0,
            0, nullptr, 0, nullptr, 1, &initialRaytraceBarrier);

        vkCmdBindPipeline(commandBuffers[i],
            VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR,
            pipeline);
        vkCmdBindDescriptorSets(commandBuffers[i], VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR,
            pipelineLayout, 0, 1, &descriptorSet, 0, nullptr);
        VkBufferDeviceAddressInfo addressInfo{};
        addressInfo.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO;
        addressInfo.buffer = shaderBindingTable;
        VkDeviceAddress shaderBindingTableAddress = vkGetBufferDeviceAddress(logicalDevice, &addressInfo);
        VkStridedDeviceAddressRegionKHR raygenShaderBindingTable{};
        raygenShaderBindingTable.deviceAddress = shaderBindingTableAddress;
        raygenShaderBindingTable.stride = groupHandleSize;
        raygenShaderBindingTable.size = groupHandleSize;
        VkStridedDeviceAddressRegionKHR missShaderBindingTable{};
        missShaderBindingTable.deviceAddress = shaderBindingTableAddress + groupSizeAligned;
        missShaderBindingTable.stride = groupHandleSize;
        missShaderBindingTable.size = groupHandleSize;
        VkStridedDeviceAddressRegionKHR hitShaderShaderBindingTable{};
        hitShaderShaderBindingTable.deviceAddress = shaderBindingTableAddress + 2u * groupSizeAligned;
        hitShaderShaderBindingTable.stride = groupHandleSize;
        hitShaderShaderBindingTable.size = groupHandleSize;
        VkStridedDeviceAddressRegionKHR callableShaderBindingTable{};
        vkCmdTraceRaysKHR(commandBuffers[i],
            &raygenShaderBindingTable,
            &missShaderBindingTable,
            &hitShaderShaderBindingTable,
            &callableShaderBindingTable,
            swapchainExtent.width,
            swapchainExtent.height,
            1
        );

        VkImageSubresourceLayers subresourceLayers{};
        subresourceLayers.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        subresourceLayers.mipLevel = 0;
        subresourceLayers.baseArrayLayer = 0;
        subresourceLayers.layerCount = 1;

        VkImageMemoryBarrier raytraceBarrier{};
        raytraceBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        raytraceBarrier.oldLayout = VK_IMAGE_LAYOUT_GENERAL;
        raytraceBarrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        raytraceBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        raytraceBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        raytraceBarrier.image = storageImage;
        raytraceBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        raytraceBarrier.subresourceRange.baseMipLevel = 0;
        raytraceBarrier.subresourceRange.levelCount = 1;
        raytraceBarrier.subresourceRange.baseArrayLayer = 0;
        raytraceBarrier.subresourceRange.layerCount = 1;
        raytraceBarrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
        raytraceBarrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
        VkImageMemoryBarrier initialBarrier{};
        initialBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        initialBarrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        initialBarrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        initialBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        initialBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        initialBarrier.image = swapchainImages[i];
        initialBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        initialBarrier.subresourceRange.baseMipLevel = 0;
        initialBarrier.subresourceRange.levelCount = 1;
        initialBarrier.subresourceRange.baseArrayLayer = 0;
        initialBarrier.subresourceRange.layerCount = 1;
        initialBarrier.srcAccessMask = 0;
        initialBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        VkImageMemoryBarrier initialBarriers[] = { raytraceBarrier, initialBarrier };
        vkCmdPipelineBarrier(commandBuffers[i], VK_PIPELINE_STAGE_RAY_TRACING_SHADER_BIT_KHR, VK_PIPELINE_STAGE_TRANSFER_BIT, 0,
            0, nullptr, 0, nullptr, 2, initialBarriers);

        VkImageBlit blitInfo{};
        blitInfo.srcSubresource = subresourceLayers;
        blitInfo.dstSubresource = subresourceLayers;
        blitInfo.dstOffsets[0] = { 0,0,0 };
        blitInfo.dstOffsets[1] = { static_cast<int32_t>(swapchainExtent.width), static_cast<int32_t>(swapchainExtent.height), 1 };
        blitInfo.srcOffsets[0] = { 0,0,0 };
        blitInfo.srcOffsets[1] = { static_cast<int32_t>(swapchainExtent.width), static_cast<int32_t>(swapchainExtent.height), 1 };
        vkCmdBlitImage(commandBuffers[i], storageImage, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, swapchainImages[i],
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &blitInfo, VK_FILTER_LINEAR);
        VkImageMemoryBarrier finalBarrier{};
        finalBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        finalBarrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        finalBarrier.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
        finalBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        finalBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        finalBarrier.image = swapchainImages[i];
        finalBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        finalBarrier.subresourceRange.baseMipLevel = 0;
        finalBarrier.subresourceRange.levelCount = 1;
        finalBarrier.subresourceRange.baseArrayLayer = 0;
        finalBarrier.subresourceRange.layerCount = 1;
        finalBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        finalBarrier.dstAccessMask = 0;
        vkCmdPipelineBarrier(commandBuffers[i], VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, 0,
            0, nullptr, 0, nullptr, 1, &finalBarrier);
        if (vkEndCommandBuffer(commandBuffers[i]) != VK_SUCCESS) {
            throw std::runtime_error("Failed to record command buffer!");
        }
    }
    
    VkSemaphoreCreateInfo semaphoreInfo{};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    vkCreateSemaphore(logicalDevice, &semaphoreInfo, nullptr, &imageAvailableSemaphore);
    vkCreateSemaphore(logicalDevice, &semaphoreInfo, nullptr, &imageRenderedSemaphore);
    VkFenceCreateInfo fenceInfo{};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    //fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
    vkCreateFence(logicalDevice, &fenceInfo, nullptr, &frameInFlight);
}

void VulkanApp::renderFrame() {

    uint32_t swapchainIndex;
    if (vkAcquireNextImageKHR(logicalDevice, swapchain, UINT64_MAX, imageAvailableSemaphore, VK_NULL_HANDLE, &swapchainIndex) != VK_SUCCESS) {
        throw std::runtime_error("Failed to present swap chain image!");
    }
    //vkWaitForFences(logicalDevice, 1, &frameInFlight, VK_TRUE, UINT64_MAX);
    //vkResetFences(logicalDevice, 1, &frameInFlight);
    
    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffers[swapchainIndex];
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = &imageAvailableSemaphore;
    submitInfo.pSignalSemaphores = &imageRenderedSemaphore;
    submitInfo.signalSemaphoreCount = 1;
    VkPipelineStageFlags flags[] = { VK_PIPELINE_STAGE_RAY_TRACING_SHADER_BIT_KHR };
    submitInfo.pWaitDstStageMask = flags;
    vkQueueSubmit(graphicsQueue, 1, &submitInfo, nullptr);
    vkQueueWaitIdle(graphicsQueue);


    VkPresentInfoKHR presentInfo{};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    VkSwapchainKHR swapchains[] = { swapchain };
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = swapchains;
    presentInfo.pImageIndices = &swapchainIndex;
    presentInfo.pWaitSemaphores = &imageRenderedSemaphore;
    presentInfo.waitSemaphoreCount = 1;
    if (vkQueuePresentKHR(presentQueue, &presentInfo) != VK_SUCCESS) {
        throw std::runtime_error("Failed to present swap chain image!");
    }
    
}

void VulkanApp::initWindow() {
    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);
    window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "Vulkan", nullptr, nullptr);
    glfwSetWindowUserPointer(window, this);
    glfwSetCursorPosCallback(window, cursor_position_callback);
}

void VulkanApp::initVulkan(){
    setup->createInstance(instance, enableValidationLayers, validationLayers, instanceExtensions, 2);
    if (enableValidationLayers) {
        setup->setupDebugMessenger(enableValidationLayers, instance, debugMessenger);
    }
    setup->createSurface(instance, window, surface);
    physicalDevice = VK_NULL_HANDLE;
    setup->pickPhysicalDevice(instance, physicalDevice, deviceExtensions, surface);
    VkPhysicalDeviceFeatures deviceFeatures{};

    VkPhysicalDeviceFeatures2 features2{ VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2 };
    VkPhysicalDeviceVulkan12Features features12{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES };
    features12.bufferDeviceAddress = VK_TRUE;
    VkPhysicalDeviceVulkan11Features features11{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES };

    features2.pNext = &features12;
    features12.pNext = &features11;
    features11.pNext = &asFeatures;
    asFeatures.pNext = &rtPipelineFeatures;
    asFeatures.accelerationStructure = VK_TRUE;
    rtPipelineFeatures.rayTracingPipeline = VK_TRUE;

    setup->createLogicalDevice(physicalDevice, logicalDevice, surface, deviceExtensions, enableValidationLayers,
        validationLayers, deviceFeatures, &features2);
    loadProxyFuncs();
    setup->getQueues(physicalDevice, logicalDevice, surface, graphicsQueue, presentQueue);
    setup->createSwapchain(physicalDevice, logicalDevice, window, surface, swapchain, swapchainExtent,
        swapchainImageFormat, swapchainImages);
    setup->createCommandPool(physicalDevice, logicalDevice, surface, commandPool);
    setup->createSwapchainImageViews(swapchainImageViews, logicalDevice, swapchainImages, 
        swapchainImageFormat, graphicsQueue, commandPool);
    tinyobj::ObjReaderConfig config;
    config.triangulate = true;
    config.mtl_search_path = "";
    config.triangulation_method = "simple";
    config.vertex_color = false;
    model.loadModel("C:/Users/juice/Documents/Graphics/Models/CornellBox/CornellBox-Original.obj", config);
    initRayTracing();
    createBuffers();
    createImageStorage();
    createAccelerationStructures();
    createUniformBuffer();
    createDescriptorSets();
    createRaytracingPipeline();
}

void VulkanApp::initRayTracing() {
    rtProperties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_PROPERTIES_KHR;
    VkPhysicalDeviceProperties2 prop2{ VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2 };
    prop2.pNext = &rtProperties;
    vkGetPhysicalDeviceProperties2(physicalDevice, &prop2);

}

void VulkanApp::cleanup() {
    vkDestroyBuffer(logicalDevice, positionBuffer, nullptr);
    vkFreeMemory(logicalDevice, positionBufferMemory, nullptr);
    vkDestroyBuffer(logicalDevice, indexBuffer, nullptr);
    vkFreeMemory(logicalDevice, indexBufferMemory, nullptr);
    vkDestroyBuffer(logicalDevice, colorBuffer, nullptr);
    vkFreeMemory(logicalDevice, colorBufferMemory, nullptr);
    vkDestroyBuffer(logicalDevice, emissionBuffer, nullptr);
    vkFreeMemory(logicalDevice, emissionBufferMemory, nullptr);
    vkDestroyBuffer(logicalDevice, blasBuffer, nullptr);
    vkFreeMemory(logicalDevice, blasBufferMemory, nullptr);
    vkDestroyBuffer(logicalDevice, blasScratchBuffer, nullptr);
    vkFreeMemory(logicalDevice, blasScratchBufferMemory, nullptr);
    vkDestroyBuffer(logicalDevice, instanceBuffer, nullptr);
    vkFreeMemory(logicalDevice, instanceBufferMemory, nullptr);
    vkDestroyBuffer(logicalDevice, tlasBuffer, nullptr);
    vkFreeMemory(logicalDevice, tlasBufferMemory, nullptr);
    vkDestroyBuffer(logicalDevice, tlasScratchBuffer, nullptr);
    vkFreeMemory(logicalDevice, tlasScratchBufferMemory, nullptr);
    vkDestroyImageView(logicalDevice, storageImageView, nullptr);
    vkDestroyImage(logicalDevice, storageImage, nullptr);
    vkFreeMemory(logicalDevice, storageImageMemory, nullptr);
    vkDestroyDescriptorSetLayout(logicalDevice, setLayout, nullptr);
    vkDestroyDescriptorPool(logicalDevice, descriptorPool, nullptr);
    vkDestroyAccelerationStructureKHR(logicalDevice, blas, nullptr);
    vkDestroyAccelerationStructureKHR(logicalDevice, tlas, nullptr);
    //vkFreeCommandBuffers(logicalDevice, commandPool, 1, &commandBuffer);
    vkDestroyCommandPool(logicalDevice, commandPool, nullptr);
    vkDestroyBuffer(logicalDevice, uniformBuffer, nullptr);
    vkFreeMemory(logicalDevice, uniformBufferMemory, nullptr);
    vkDestroyPipelineLayout(logicalDevice, pipelineLayout, nullptr);
    vkDestroyPipeline(logicalDevice, pipeline, nullptr);
    vkDestroyBuffer(logicalDevice, shaderBindingTable, nullptr);
    vkFreeMemory(logicalDevice, shaderBindingTableMemory, nullptr);
    vkDestroySemaphore(logicalDevice, imageAvailableSemaphore, nullptr);
    for (VkImageView& swapchainImageView : swapchainImageViews) {
        vkDestroyImageView(logicalDevice, swapchainImageView, nullptr);
    }
    vkDestroySwapchainKHR(logicalDevice, swapchain, nullptr);
    vkDestroySurfaceKHR(instance, surface, nullptr);
    if (enableValidationLayers) {
        vkDestroyDebugUtilsMessengerEXT(instance, debugMessenger, nullptr);
    }
    vkDestroyDevice(logicalDevice, nullptr);
    vkDestroyInstance(instance, nullptr);
    glfwDestroyWindow(window);
    glfwTerminate();
}