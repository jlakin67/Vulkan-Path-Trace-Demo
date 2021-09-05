#pragma once
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <vector>
#include <iostream>
#include <fstream>

std::vector<char> readSourceFile(const std::string& filename);

VkShaderModule createShaderModule(VkDevice logicalDevice, const std::vector<char>& code);

VkCommandBuffer beginSingleTimeCommands(VkDevice logicalDevice, VkCommandPool commandPool);

void endSingleTimeCommands(VkDevice logicalDevice, VkQueue graphicsQueue,
    VkCommandPool commandPool, VkCommandBuffer commandBuffer);

uint32_t findMemoryType(VkPhysicalDevice physicalDevice, uint32_t typeFilter, VkMemoryPropertyFlags properties);

VkFormat findSupportedFormat(VkPhysicalDevice physicalDevice,
    const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features);

bool hasStencilComponent(VkFormat format);

VkFormat findDepthFormat(VkPhysicalDevice physicalDevice);

void transitionImageLayout(VkDevice logicalDevice, VkQueue graphicsQueue,
    VkCommandPool commandPool, VkImage image, VkFormat format,
    VkImageLayout oldLayout, VkImageLayout newLayout, uint32_t mipLevels);

void createImage(VkPhysicalDevice physicalDevice, VkDevice logicalDevice, 
    uint32_t width, uint32_t height, uint32_t mipLevels, VkFormat format, VkImageTiling tiling,
    VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory);

VkImageView createImageView(VkDevice logicalDevice, VkImage image, 
    VkFormat format, VkImageAspectFlags aspectFlags, uint32_t mipLevels);

void createBuffer(VkPhysicalDevice physicalDevice, VkDevice logicalDevice, VkDeviceSize size, VkBufferUsageFlags usage, 
    VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory);

void copyBuffer(VkDevice logicalDevice, VkQueue graphicsQueue,
    VkCommandPool commandPool, VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);

void copyBufferToImage(VkDevice logicalDevice, VkQueue graphicsQueue,
    VkCommandPool commandPool, VkBuffer buffer, VkImage image, uint32_t width, uint32_t height);

VkResult vkCreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo,
    const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger);

void vkDestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger,
    const VkAllocationCallbacks* pAllocator);

VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT messageType,
    const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
    void* pUserData);