#include <cassert>
#include <vulkan/vulkan.h>

namespace vkh {
struct ApplicationInfo : public VkApplicationInfo {
  ApplicationInfo() {
    pApplicationName = "";
    sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    pNext = nullptr;
  }
};
struct InstanceCreateInfo : public VkInstanceCreateInfo {
  InstanceCreateInfo() {
    sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    pNext = nullptr;
    flags = 0;
  }
};

float kQueuePriorities[] = {1};
struct DeviceQueueCreateInfo : public VkDeviceQueueCreateInfo {
  DeviceQueueCreateInfo() {
    sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    pNext = nullptr;
    flags = 0;
    pQueuePriorities = kQueuePriorities;
  }
};

struct DeviceCreateInfo : public VkDeviceCreateInfo {
  DeviceCreateInfo() {
    sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    pNext = nullptr;
    enabledExtensionCount = 0;
    ppEnabledExtensionNames = nullptr;
    pEnabledFeatures = nullptr;
  }
};

// I personally don't believe in allocators
VkInstance CreateInstance(const VkInstanceCreateInfo& create_info) {
  VkInstance instance;
  assert(vkCreateInstance(&create_info, nullptr, &instance) == VK_SUCCESS);
  return instance;
}

VkDevice CreateDevice(VkPhysicalDevice physical_device, const VkDeviceCreateInfo& create_info) {
  VkDevice device;
  assert(vkCreateDevice(physical_device, &create_info, nullptr, &device) == VK_SUCCESS);
  return device;
}

VkQueue GetDeviceQueue(VkDevice device, uint32_t queue_family_index, uint32_t queue_index) {
  VkQueue queue;
  vkGetDeviceQueue(device, queue_family_index, queue_index, &queue);
  return queue;
}

VkBool32 GetPhysicalDeviceSurfaceSupportKHR(VkPhysicalDevice physical_device, uint32_t queue_family_index, VkSurfaceKHR surface) {
  VkBool32 supported;
  assert(vkGetPhysicalDeviceSurfaceSupportKHR(physical_device, queue_family_index, surface, &supported) == VK_SUCCESS);
  return supported;
}

VkPhysicalDeviceProperties GetPhysicalDeviceProperties(VkPhysicalDevice physical_device) {
  VkPhysicalDeviceProperties properties;
  vkGetPhysicalDeviceProperties(physical_device, &properties);
  return properties;
}

VkSurfaceCapabilitiesKHR GetPhysicalDeviceSurfaceCapabilitiesKHR(VkPhysicalDevice physical_device, VkSurfaceKHR surface) {
  VkSurfaceCapabilitiesKHR capabilities;
  assert(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physical_device, surface, &capabilities) == VK_SUCCESS);
  return capabilities;
}
}

int32_t GetQueueFamilySupportingSurface(VkPhysicalDevice physical_device, VkSurfaceKHR surface) {
    uint32_t num_families = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &num_families, nullptr);

    for (uint32_t i = 0; i < num_families; ++i) {
      if(vkh::GetPhysicalDeviceSurfaceSupportKHR(physical_device, i, surface)) {
        return i;
      }
    }

    return -1;
}

#define GET_MACRO(var, _1, _2, _3, _4, _5, _6, NAME, ...) NAME
#define F(...) GET_MACRO(__VA_ARGS__, FILL6, FILL5, FILL4, FILL3, FILL2, FILL1)(__VA_ARGS__)

#define FILL1(var, assign1) \
  var; \
  var.assign1;

#define FILL2(var, assign1, assign2) \
  var; \
  var.assign1; \
  var.assign2;

#define FILL3(var, assign1, assign2, assign3) \
  var; \
  var.assign1; \
  var.assign2; \
  var.assign3;

#define FILL4(var, assign1, assign2, assign3, assign4) \
  var; \
  var.assign1; \
  var.assign2; \
  var.assign3; \
  var.assign4;

#define FILL5(var, assign1, assign2, assign3, assign4, assign5) \
  var; \
  var.assign1; \
  var.assign2; \
  var.assign3; \
  var.assign4; \
  var.assign5;

#define FILL6(var, assign1, assign2, assign3, assign4, assign5, assign6) \
  var; \
  var.assign1; \
  var.assign2; \
  var.assign3; \
  var.assign4; \
  var.assign5; \
  var.assign6;
