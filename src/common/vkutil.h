#pragma once

#include "glad/vulkan.h"

#include <functional>
#include <vector>

VkShaderModule createShaderModule(VkDevice device, const std::vector<uint8_t>& code);
void executeOneShotCommandBufferOnQueue(VkDevice device, std::function<void(VkCommandBuffer)> func, int queueIndex);
void writeToGpuMemory(VkDevice device, VkDeviceMemory memory, const void* src, size_t size);
uint32_t findMemoryType(VkPhysicalDevice physicalDevice, uint32_t typeFilter, VkMemoryPropertyFlags properties);
