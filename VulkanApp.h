#pragma once
#include "VulkanSetup.h"
#include <vector>
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <array>
#include "Model.h"
#include "Camera.h"

extern double currentTime, lastTime, deltaTime;

#ifdef NDEBUG
const bool enableValidationLayers = false;
#else
const bool enableValidationLayers = true;
#endif

constexpr uint32_t SCR_WIDTH = 1600, SCR_HEIGHT = 900;
const std::vector<const char*> validationLayers = {
	"VK_LAYER_KHRONOS_validation"
};

const std::vector<const char*> instanceExtensions{
};

const std::vector<const char*> deviceExtensions = {
	VK_KHR_SWAPCHAIN_EXTENSION_NAME,
	VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME,
	VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME,
	VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME,
};

struct SceneProperties {
	glm::mat4 viewInverse;
	glm::mat4 projInverse;
};

class VulkanApp {
public:
	void run();

	//PFN_vkGetBufferDeviceAddressKHR vkGetBufferDeviceAddressKHR;
	PFN_vkCreateAccelerationStructureKHR vkCreateAccelerationStructureKHR;
	PFN_vkDestroyAccelerationStructureKHR vkDestroyAccelerationStructureKHR;
	PFN_vkGetAccelerationStructureBuildSizesKHR vkGetAccelerationStructureBuildSizesKHR;
	PFN_vkGetAccelerationStructureDeviceAddressKHR vkGetAccelerationStructureDeviceAddressKHR;
	PFN_vkCmdBuildAccelerationStructuresKHR vkCmdBuildAccelerationStructuresKHR;
	PFN_vkBuildAccelerationStructuresKHR vkBuildAccelerationStructuresKHR;
	PFN_vkCmdTraceRaysKHR vkCmdTraceRaysKHR;
	PFN_vkGetRayTracingShaderGroupHandlesKHR vkGetRayTracingShaderGroupHandlesKHR;
	PFN_vkCreateRayTracingPipelinesKHR vkCreateRayTracingPipelinesKHR;

private:
	VulkanSetup* setup;
	GLFWwindow* window;
	VkInstance instance;
	VkDebugUtilsMessengerEXT debugMessenger;
	VkSurfaceKHR surface;
	VkPhysicalDevice physicalDevice;
	VkDevice logicalDevice;
	VkQueue graphicsQueue, presentQueue;
	VkSwapchainKHR swapchain;
	VkExtent2D swapchainExtent; VkFormat swapchainImageFormat;
	std::vector<VkImage> swapchainImages; std::vector<VkImageView> swapchainImageViews;
	VkPhysicalDeviceAccelerationStructureFeaturesKHR asFeatures{ VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_FEATURES_KHR };
	VkPhysicalDeviceRayTracingPipelineFeaturesKHR rtPipelineFeatures{ VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_FEATURES_KHR };
	VkPhysicalDeviceRayTracingPipelinePropertiesKHR  rayTracingPipelineProperties{};
	Model model;
	VkBuffer positionBuffer; VkDeviceMemory positionBufferMemory;
	VkBuffer indexBuffer; VkDeviceMemory indexBufferMemory;
	VkDeviceAddress positionBufferAddress; VkDeviceAddress indexBufferAddress;
	VkBuffer colorBuffer; VkDeviceMemory colorBufferMemory;
	VkBuffer emissionBuffer; VkDeviceMemory emissionBufferMemory;
	VkCommandPool commandPool;
	VkBuffer blasBuffer; VkDeviceMemory blasBufferMemory;
	VkAccelerationStructureKHR blas;
	VkBuffer blasScratchBuffer; VkDeviceMemory blasScratchBufferMemory;
	VkBuffer instanceBuffer; VkDeviceMemory instanceBufferMemory;
	VkBuffer tlasBuffer; VkDeviceMemory tlasBufferMemory;
	VkAccelerationStructureKHR tlas;
	VkBuffer tlasScratchBuffer; VkDeviceMemory tlasScratchBufferMemory;
	VkImage storageImage; VkDeviceMemory storageImageMemory; VkImageView storageImageView;
	//VkSampler sampler;
	VkDescriptorSetLayout setLayout;
	VkBuffer uniformBuffer; VkDeviceMemory uniformBufferMemory;
	Camera camera;
	SceneProperties scene;
	VkDescriptorPool descriptorPool; VkDescriptorSet descriptorSet;
	VkPipelineLayout pipelineLayout; VkPipeline pipeline;
	VkPhysicalDeviceRayTracingPipelinePropertiesKHR rtProperties;
	VkBuffer shaderBindingTable; VkDeviceMemory shaderBindingTableMemory;
	VkCommandBuffer commandBuffer; std::vector<VkCommandBuffer> blitCommandBuffers;
	VkSemaphore imageAvailableSemaphore;

	void mainLoop();
	void initWindow();
	void initVulkan();
	void initRayTracing();
	void cleanup();
	void loadProxyFuncs();
	void createBuffers();
	void createAccelerationStructures();
	void createUniformBuffer();
	void updateUniformBuffer();
	void createImageStorage();
	void createDescriptorSets();
	void createRaytracingPipeline();
	void renderFrame();
};