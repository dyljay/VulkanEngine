#include "engine_device.hpp"
#include "SDL3/SDL_vulkan.h"
#include "vulkan/vulkan_core.h"

// std headers
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <set>
#include <stdexcept>
#include <unordered_set>

namespace GameEngine {

// local callback functions
static VKAPI_ATTR VkBool32 VKAPI_CALL
debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
              VkDebugUtilsMessageTypeFlagsEXT messageType,
              const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData,
              void *pUserData) {
  std::cerr << "validation layer: " << pCallbackData->pMessage << std::endl;

  return VK_FALSE;
}

VkResult CreateDebugUtilsMessengerEXT(
    VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT *pCreateInfo,
    const VkAllocationCallbacks *pAllocator,
    VkDebugUtilsMessengerEXT *pDebugMessenger) {
  auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(
      instance, "vkCreateDebugUtilsMessengerEXT");
  if (func != nullptr) {
    return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
  } else {
    return VK_ERROR_EXTENSION_NOT_PRESENT;
  }
}

void DestroyDebugUtilsMessengerEXT(VkInstance instance,
                                   VkDebugUtilsMessengerEXT debugMessenger,
                                   const VkAllocationCallbacks *pAllocator) {
  auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(
      instance, "vkDestroyDebugUtilsMessengerEXT");
  if (func != nullptr) {
    func(instance, debugMessenger, pAllocator);
  }
}

// class member functions
EngineDevice::EngineDevice(EngineWindow &window) : window{window} {
  createInstance();
  setupDebugMessenger();
  createSurface();
  pickPhysicalDevice();
  setMaxSampleCount();
  createLogicalDevice();
  createCommandPool();
  createAllocator();
}

EngineDevice::~EngineDevice() {
  vmaDestroyAllocator(allocator_);
  vkDestroyCommandPool(device_, commandPool, nullptr);
  vkDestroyDevice(device_, nullptr);

  if (enableValidationLayers) {
    DestroyDebugUtilsMessengerEXT(instance_, debugMessenger, nullptr);
  }

  vkDestroySurfaceKHR(instance_, surface_, nullptr);
  vkDestroyInstance(instance_, nullptr);
}

void EngineDevice::createInstance() {
  if (enableValidationLayers && !checkValidationLayerSupport()) {
    throw std::runtime_error("validation layers requested, but not available!");
  }

  VkApplicationInfo appInfo = {};
  appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
  appInfo.pApplicationName = "Vulkan Game Engine";
  appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
  appInfo.pEngineName = "No Engine";
  appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
  appInfo.apiVersion = VK_API_VERSION_1_4;

  VkInstanceCreateInfo createInfo = {};
  createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
  createInfo.pApplicationInfo = &appInfo;

  auto extensions = getRequiredExtensions();

#if defined __APPLE__ && defined __arm64__
  extensions.emplace_back(VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME);
  extensions.emplace_back(
      VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);
  createInfo.flags |= VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;
#endif

  createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
  createInfo.ppEnabledExtensionNames = extensions.data();

  VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo;
  if (enableValidationLayers) {
    createInfo.enabledLayerCount =
        static_cast<uint32_t>(validationLayers.size());
    createInfo.ppEnabledLayerNames = validationLayers.data();

    populateDebugMessengerCreateInfo(debugCreateInfo);
    createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT *)&debugCreateInfo;
  } else {
    createInfo.enabledLayerCount = 0;
    createInfo.pNext = nullptr;
  }

  if (vkCreateInstance(&createInfo, nullptr, &instance_) != VK_SUCCESS) {
    throw std::runtime_error("failed to create instance!");
  }

  hasSDL3RequiredInstanceExtensions();
}

void EngineDevice::pickPhysicalDevice() {
  uint32_t deviceCount = 0;
  vkEnumeratePhysicalDevices(instance_, &deviceCount, nullptr);
  if (deviceCount == 0) {
    throw std::runtime_error("failed to find GPUs with Vulkan support!");
  }
  std::cout << "Device count: " << deviceCount << std::endl;
  std::vector<VkPhysicalDevice> devices(deviceCount);
  vkEnumeratePhysicalDevices(instance_, &deviceCount, devices.data());

  for (const auto &device : devices) {
    if (isDeviceSuitable(device)) {
      physicalDevice = device;
      break;
    }
  }

  if (physicalDevice == VK_NULL_HANDLE) {
    throw std::runtime_error("failed to find a suitable GPU!");
  }

  vkGetPhysicalDeviceProperties(physicalDevice, &properties);
  std::cout << "physical device: " << properties.deviceName << std::endl;
}

void EngineDevice::setMaxSampleCount() {
  VkSampleCountFlags countBits =
      properties.limits.framebufferColorSampleCounts &
      properties.limits.framebufferDepthSampleCounts;

  if (countBits & VK_SAMPLE_COUNT_4_BIT) {
    msaaSamples_ = VK_SAMPLE_COUNT_4_BIT;
  } else {
    msaaSamples_ = VK_SAMPLE_COUNT_1_BIT;
  }
}

void EngineDevice::createLogicalDevice() {
  QueueFamilyIndices indices = findQueueFamilies(physicalDevice);

  std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
  std::set<uint32_t> uniqueQueueFamilies = {indices.graphicsFamily,
                                            indices.presentFamily};

  float queuePriority = 1.0f;
  for (uint32_t queueFamily : uniqueQueueFamilies) {
    VkDeviceQueueCreateInfo queueCreateInfo = {};
    queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queueCreateInfo.queueFamilyIndex = queueFamily;
    queueCreateInfo.queueCount = 1;
    queueCreateInfo.pQueuePriorities = &queuePriority;
    queueCreateInfos.push_back(queueCreateInfo);
  }

  VkPhysicalDeviceFeatures deviceFeatures = {};
  deviceFeatures.samplerAnisotropy = VK_TRUE;

  VkDeviceCreateInfo createInfo = {};
  createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;

  createInfo.queueCreateInfoCount =
      static_cast<uint32_t>(queueCreateInfos.size());
  createInfo.pQueueCreateInfos = queueCreateInfos.data();

  createInfo.pEnabledFeatures = &deviceFeatures;
  createInfo.enabledExtensionCount =
      static_cast<uint32_t>(deviceExtensions.size());
  createInfo.ppEnabledExtensionNames = deviceExtensions.data();

  // might not really be necessary anymore because device specific validation
  // layers have been deprecated
  if (enableValidationLayers) {
    createInfo.enabledLayerCount =
        static_cast<uint32_t>(validationLayers.size());
    createInfo.ppEnabledLayerNames = validationLayers.data();
  } else {
    createInfo.enabledLayerCount = 0;
  }

  VkPhysicalDeviceBufferDeviceAddressFeatures bdaFeatures{};
  bdaFeatures.sType =
      VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_BUFFER_DEVICE_ADDRESS_FEATURES;
  bdaFeatures.bufferDeviceAddress = VK_TRUE;

  VkPhysicalDeviceDescriptorIndexingFeatures descriptorIndexing{};
  descriptorIndexing.sType =
      VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_FEATURES;
  descriptorIndexing.shaderSampledImageArrayNonUniformIndexing = VK_TRUE;
  descriptorIndexing.runtimeDescriptorArray = VK_TRUE;
  descriptorIndexing.descriptorBindingVariableDescriptorCount = VK_TRUE;
  descriptorIndexing.descriptorBindingPartiallyBound = VK_TRUE;
  descriptorIndexing.descriptorBindingSampledImageUpdateAfterBind = VK_TRUE;
  descriptorIndexing.descriptorBindingUpdateUnusedWhilePending = VK_TRUE;
  descriptorIndexing.pNext = &bdaFeatures;

  // adding descriptor indexing features struct to device create extension
  createInfo.pNext = &descriptorIndexing;

  if (vkCreateDevice(physicalDevice, &createInfo, nullptr, &device_) !=
      VK_SUCCESS) {
    throw std::runtime_error("failed to create logical device!");
  }

  vkGetDeviceQueue(device_, indices.graphicsFamily, 0, &graphicsQueue_);
  vkGetDeviceQueue(device_, indices.presentFamily, 0, &presentQueue_);
}

void EngineDevice::createCommandPool() {
  QueueFamilyIndices queueFamilyIndices = findPhysicalQueueFamilies();

  VkCommandPoolCreateInfo poolInfo = {};
  poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
  poolInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily;
  poolInfo.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT |
                   VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

  if (vkCreateCommandPool(device_, &poolInfo, nullptr, &commandPool) !=
      VK_SUCCESS) {
    throw std::runtime_error("failed to create command pool!");
  }
}

void EngineDevice::createAllocator() {
  VmaAllocatorCreateInfo allocatorInfo{};
  allocatorInfo.physicalDevice = physicalDevice;
  allocatorInfo.device = device_;
  allocatorInfo.instance = instance_;
  allocatorInfo.flags = VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT;

  vmaCreateAllocator(&allocatorInfo, &allocator_);
}

void EngineDevice::createSurface() {
  window.createWindowSurface(instance_, &surface_);
}

bool EngineDevice::isDeviceSuitable(VkPhysicalDevice device) {
  QueueFamilyIndices indices = findQueueFamilies(device);

  bool extensionsSupported = checkDeviceExtensionSupport(device);

  bool swapChainAdequate = false;
  if (extensionsSupported) {
    SwapChainSupportDetails swapChainSupport = querySwapChainSupport(device);
    swapChainAdequate = !swapChainSupport.formats.empty() &&
                        !swapChainSupport.presentModes.empty();
  }

  VkPhysicalDeviceFeatures supportedFeatures;
  vkGetPhysicalDeviceFeatures(device, &supportedFeatures);

  return indices.isComplete() && extensionsSupported && swapChainAdequate &&
         supportedFeatures.samplerAnisotropy;
}

void EngineDevice::populateDebugMessengerCreateInfo(
    VkDebugUtilsMessengerCreateInfoEXT &createInfo) {
  createInfo = {};
  createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
  createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                               VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
  createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                           VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                           VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
  createInfo.pfnUserCallback = debugCallback;
  createInfo.pUserData = nullptr; // Optional
}

void EngineDevice::setupDebugMessenger() {
  if (!enableValidationLayers)
    return;
  VkDebugUtilsMessengerCreateInfoEXT createInfo;
  populateDebugMessengerCreateInfo(createInfo);
  if (CreateDebugUtilsMessengerEXT(instance_, &createInfo, nullptr,
                                   &debugMessenger) != VK_SUCCESS) {
    throw std::runtime_error("failed to set up debug messenger!");
  }
}

bool EngineDevice::checkValidationLayerSupport() {
  uint32_t layerCount;
  vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

  std::vector<VkLayerProperties> availableLayers(layerCount);
  vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

  for (const char *layerName : validationLayers) {
    bool layerFound = false;

    for (const auto &layerProperties : availableLayers) {
      if (strcmp(layerName, layerProperties.layerName) == 0) {
        layerFound = true;
        break;
      }
    }

    if (!layerFound) {
      return false;
    }
  }

  return true;
}

std::vector<const char *> EngineDevice::getRequiredExtensions() {
  uint32_t sdl3ExtensionCount = 0;
  const char *const *sdl3Extensions;

  sdl3Extensions = SDL_Vulkan_GetInstanceExtensions(&sdl3ExtensionCount);

  std::vector<const char *> extensions(sdl3Extensions,
                                       sdl3Extensions + sdl3ExtensionCount);

  if (enableValidationLayers) {
    extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
  }

  return extensions;
}

void EngineDevice::hasSDL3RequiredInstanceExtensions() {
  uint32_t extensionCount = 0;
  vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);
  std::vector<VkExtensionProperties> extensions(extensionCount);
  vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount,
                                         extensions.data());

  // might be useful later, but for now it just clutters the terminal
  /*
  std::cout << "available extensions:" << std::endl;
  std::unordered_set<std::string> available;
  for (const auto &extension : extensions) {
    std::cout << "\t" << extension.extensionName << std::endl;
    available.insert(extension.extensionName);
  }

  std::cout << "required extensions:" << std::endl;
  auto requiredExtensions = getRequiredExtensions();
  for (const auto &required : requiredExtensions) {
    std::cout << "\t" << required << std::endl;
    if (available.find(required) == available.end()) {
      throw std::runtime_error("Missing required SDL3 extension");
    }
  }
  */
}

bool EngineDevice::checkDeviceExtensionSupport(VkPhysicalDevice device) {
  uint32_t extensionCount;
  vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount,
                                       nullptr);

  std::vector<VkExtensionProperties> availableExtensions(extensionCount);
  vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount,
                                       availableExtensions.data());

  std::set<std::string> requiredExtensions(deviceExtensions.begin(),
                                           deviceExtensions.end());

  for (const auto &extension : availableExtensions) {
    requiredExtensions.erase(extension.extensionName);
  }

  return requiredExtensions.empty();
}

QueueFamilyIndices EngineDevice::findQueueFamilies(VkPhysicalDevice device) {
  QueueFamilyIndices indices;

  uint32_t queueFamilyCount = 0;
  vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

  std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
  vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount,
                                           queueFamilies.data());

  int i = 0;
  for (const auto &queueFamily : queueFamilies) {
    if (queueFamily.queueCount > 0 &&
        queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
      indices.graphicsFamily = i;
      indices.graphicsFamilyHasValue = true;
    }
    VkBool32 presentSupport = false;
    vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface_, &presentSupport);
    if (queueFamily.queueCount > 0 && presentSupport) {
      indices.presentFamily = i;
      indices.presentFamilyHasValue = true;
    }
    if (indices.isComplete()) {
      break;
    }

    i++;
  }

  return indices;
}

SwapChainSupportDetails
EngineDevice::querySwapChainSupport(VkPhysicalDevice device) {
  SwapChainSupportDetails details;
  vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface_,
                                            &details.capabilities);

  uint32_t formatCount;
  vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface_, &formatCount, nullptr);

  if (formatCount != 0) {
    details.formats.resize(formatCount);
    vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface_, &formatCount,
                                         details.formats.data());
  }

  uint32_t presentModeCount;
  vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface_, &presentModeCount,
                                            nullptr);

  if (presentModeCount != 0) {
    details.presentModes.resize(presentModeCount);
    vkGetPhysicalDeviceSurfacePresentModesKHR(
        device, surface_, &presentModeCount, details.presentModes.data());
  }
  return details;
}

VkFormat
EngineDevice::findSupportedFormat(const std::vector<VkFormat> &candidates,
                                  VkImageTiling tiling,
                                  VkFormatFeatureFlags features) {
  for (VkFormat format : candidates) {
    VkFormatProperties props;
    vkGetPhysicalDeviceFormatProperties(physicalDevice, format, &props);

    if (tiling == VK_IMAGE_TILING_LINEAR &&
        (props.linearTilingFeatures & features) == features) {
      return format;
    } else if (tiling == VK_IMAGE_TILING_OPTIMAL &&
               (props.optimalTilingFeatures & features) == features) {
      return format;
    }
  }
  throw std::runtime_error("failed to find supported format!");
}

uint32_t EngineDevice::findMemoryType(uint32_t typeFilter,
                                      VkMemoryPropertyFlags properties) {
  VkPhysicalDeviceMemoryProperties memProperties;
  vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);
  for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
    if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags &
                                    properties) == properties) {
      return i;
    }
  }

  throw std::runtime_error("failed to find suitable memory type!");
}

void EngineDevice::createBuffer(VkDeviceSize size, VkBufferUsageFlags usage,
                                VmaMemoryUsage memoryUsage, VkBuffer &buffer,
                                VmaAllocation &allocation,
                                VmaAllocationInfo &allocInfo) {

  VkBufferCreateInfo bufferInfo{};
  bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
  bufferInfo.size = size;
  bufferInfo.usage = usage;
  bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

  VmaAllocationCreateInfo vmaAllocInfo{};
  vmaAllocInfo.usage = memoryUsage;
  vmaAllocInfo.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT;

  vmaCreateBuffer(allocator_, &bufferInfo, &vmaAllocInfo, &buffer, &allocation,
                  &allocInfo);
}

VkCommandBuffer EngineDevice::beginSingleTimeCommands() {
  VkCommandBufferAllocateInfo allocInfo{};
  allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  allocInfo.commandPool = commandPool;
  allocInfo.commandBufferCount = 1;

  VkCommandBuffer commandBuffer;
  vkAllocateCommandBuffers(device_, &allocInfo, &commandBuffer);

  VkCommandBufferBeginInfo beginInfo{};
  beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

  vkBeginCommandBuffer(commandBuffer, &beginInfo);
  return commandBuffer;
}

void EngineDevice::endSingleTimeCommands(VkCommandBuffer commandBuffer) {
  vkEndCommandBuffer(commandBuffer);

  VkSubmitInfo submitInfo{};
  submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
  submitInfo.commandBufferCount = 1;
  submitInfo.pCommandBuffers = &commandBuffer;

  vkQueueSubmit(graphicsQueue_, 1, &submitInfo, VK_NULL_HANDLE);
  vkQueueWaitIdle(graphicsQueue_);

  vkFreeCommandBuffers(device_, commandPool, 1, &commandBuffer);
}

void EngineDevice::copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer,
                              VkDeviceSize size, VkDeviceSize srcOffset) {
  VkCommandBuffer commandBuffer = beginSingleTimeCommands();

  VkBufferCopy copyRegion{};
  copyRegion.srcOffset = srcOffset; // Optional
  copyRegion.dstOffset = 0;         // Optional
  copyRegion.size = size;
  vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);

  endSingleTimeCommands(commandBuffer);
}

void EngineDevice::copyBufferToImage(VkBuffer buffer, VkImage image,
                                     uint32_t width, uint32_t height,
                                     uint32_t layerCount) {
  VkCommandBuffer commandBuffer = beginSingleTimeCommands();

  VkBufferImageCopy region{};
  region.bufferOffset = 0;
  region.bufferRowLength = 0;
  region.bufferImageHeight = 0;

  region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  region.imageSubresource.mipLevel = 0;
  region.imageSubresource.baseArrayLayer = 0;
  region.imageSubresource.layerCount = layerCount;

  region.imageOffset = {0, 0, 0};
  region.imageExtent = {width, height, 1};

  vkCmdCopyBufferToImage(commandBuffer, buffer, image,
                         VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
  endSingleTimeCommands(commandBuffer);
}

void EngineDevice::copyImageToBuffer(VkImage image, VkBuffer buffer,
                                     VkImageLayout imageLayout, uint32_t width,
                                     uint32_t height, uint32_t layerCount) {
  VkCommandBuffer commandBuffer = beginSingleTimeCommands();

  VkBufferImageCopy copyInfo{};
  copyInfo.bufferOffset = 0;
  copyInfo.bufferRowLength = 0;
  copyInfo.bufferImageHeight = 0;

  copyInfo.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  copyInfo.imageSubresource.mipLevel = 0;
  copyInfo.imageSubresource.baseArrayLayer = 0;
  copyInfo.imageSubresource.layerCount = layerCount;

  copyInfo.imageOffset = {0, 0, 0};
  copyInfo.imageExtent = {width, height};

  vkCmdCopyImageToBuffer(commandBuffer, image, imageLayout, buffer, 1,
                         &copyInfo);

  endSingleTimeCommands(commandBuffer);
}

void EngineDevice::createImageWithInfo(const VkImageCreateInfo &imageInfo,
                                       VkMemoryPropertyFlags properties,
                                       VkImage &image,
                                       VmaAllocation &allocation,
                                       VmaMemoryUsage usage) {

  VmaAllocationCreateInfo allocInfo{};
  allocInfo.usage = usage;
  allocInfo.requiredFlags = properties;

  auto result = vmaCreateImage(allocator_, &imageInfo, &allocInfo, &image,
                               &allocation, nullptr);

  if (result != VK_SUCCESS) {
    throw std::runtime_error("failed to create image");
  }
}
} // namespace GameEngine
