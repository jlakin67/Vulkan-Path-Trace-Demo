#pragma once
#include "util.h"
#include <set>
#include <optional>
#include <algorithm>

struct SwapChainSupportDetails {
    VkSurfaceCapabilitiesKHR capabilities;
    std::vector<VkSurfaceFormatKHR> formats;
    std::vector<VkPresentModeKHR> presentModes;
};

struct QueueFamilyIndices {
    std::optional<uint32_t> graphicsFamily;
    std::optional<uint32_t> presentFamily;
    bool isComplete() {
        return graphicsFamily.has_value() && presentFamily.has_value();
    }
};

class VulkanSetup {
public:
    static VulkanSetup* getInstance();
    VulkanSetup(VulkanSetup const&) = delete;
    VulkanSetup(VulkanSetup&&) = delete;
    VulkanSetup& operator=(VulkanSetup const&) = delete;
    VulkanSetup& operator=(VulkanSetup&&) = delete;
    ~VulkanSetup() { delete instance; }

    void createInstance(VkInstance& instance, bool enableValidationLayers, const std::vector<const char*>& validationLayers,
        const std::vector<const char*>& instanceExtensions, uint16_t minor_version);

    void setupDebugMessenger(bool enableValidationLayers, VkInstance instance,
        VkDebugUtilsMessengerEXT& debugMessenger);

    void createSurface(VkInstance instance, GLFWwindow* window, VkSurfaceKHR& surface);

    //swap chain is usually a needed device extension
    //the queue families it checks for are graphics and present
    void pickPhysicalDevice(VkInstance instance, VkPhysicalDevice& physicalDevice,
        const std::vector<const char*>& deviceExtensions, VkSurfaceKHR surface);

    //swap chain is usually a needed device extension
    //allocates 1 graphics queue and 1 present queue with priorities 1.0
    void createLogicalDevice(VkPhysicalDevice physicalDevice, VkDevice& logicalDevice,
        VkSurfaceKHR surface, const std::vector<const char*>& deviceExtensions, bool enableValidationLayers,
        const std::vector<const char*>& validationLayers, VkPhysicalDeviceFeatures deviceFeatures, void* pNext = nullptr);

    //for now just 1 graphics queue and 1 present queue, compute queue later
    void getQueues(VkPhysicalDevice physicalDevice, VkDevice logicalDevice,
        VkSurfaceKHR surface, VkQueue& graphicsQueue, VkQueue& presentQueue);

    void createSwapchain(VkPhysicalDevice physicalDevice, VkDevice logicalDevice,
        GLFWwindow* window, VkSurfaceKHR surface, VkSwapchainKHR& swapchain, VkExtent2D& swapchainExtent,
        VkFormat& swapchainImageFormat, std::vector<VkImage>& swapchainImages);

    void createSwapchainImageViews(std::vector<VkImageView>& swapchainImageViews,
        VkDevice logicalDevice, std::vector<VkImage> swapchainImages, VkFormat swapchainImageFormat,
        VkQueue graphicsQueue, VkCommandPool commandPool);

    //create graphics command pool
    void createCommandPool(VkPhysicalDevice physicalDevice, VkDevice logicalDevice,
        VkSurfaceKHR surface, VkCommandPool& commandPool);

    void createDepthResources();

private:
    VulkanSetup() = default;
    static VulkanSetup* instance;
    bool checkValidationLayerSupport(const std::vector<const char*>& validationLayers);

    void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo);

    std::vector<const char*> getRequiredExtensions(bool enableValidationLayers);

    bool checkDeviceExtensionsSupport(const std::vector<const char*>& deviceExtensions, VkPhysicalDevice physicalDevice);

    QueueFamilyIndices findQueueFamilies(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface);

    SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface);

    bool isDeviceSuitable(const std::vector<const char*>& deviceExtensions, VkPhysicalDevice physicalDevice, VkSurfaceKHR surface);

    VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);

    VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes);

    VkExtent2D chooseSwapExtent(GLFWwindow* window, const VkSurfaceCapabilitiesKHR& capabilities);
};