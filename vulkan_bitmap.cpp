#include "vulkan_util.h"
#include <cassert>
#include <iostream>
#include <set>
#include <tuple>
#include <type_traits>
#include <vector>
#include <vulkan/vulkan.h>

#include <SDL2/SDL.h>
#include <SDL2/SDL_vulkan.h>

template<class T>
struct function_traits : function_traits<decltype(&T::operator())> {};

template<class R, class... Args>
struct function_traits<R (*)(Args...)> {
  using result_type = R;
  using argument_types = std::tuple<Args...>;
};

template<class func_t, int v>
using arg_type = typename std::tuple_element<v, typename function_traits<func_t>::argument_types>::type;

template<class func_t, int v>
using arg_value_type = typename std::remove_pointer<arg_type<func_t, v>>::type;

// Note: GetProp could return a VK_ERROR that isn't caught here. Some functions
// matching this pattern can throw errors and some can't, rip.
template <typename key_t, typename function_t>
std::vector<arg_value_type<function_t, 2>> GetProps(key_t key, const function_t &GetProp) {
  uint32_t num_things = 0;
  GetProp(key, &num_things, nullptr);

  std::vector<arg_value_type<function_t, 2>> things(num_things);
  GetProp(key, &num_things, things.data());
  return things;
}

template <typename key1_t, typename key2_t, typename function_t>
std::vector<arg_value_type<function_t, 3>> GetProps(key1_t key1, key2_t key2, const function_t &GetProp) {
  uint32_t num_things = 0;
  GetProp(key1, key2, &num_things, nullptr);

  std::vector<arg_value_type<function_t, 3>> things(num_things);
  GetProp(key1, key2, &num_things, things.data());
  return things;
}

VkResult DefaultDeviceExtensionProperties(VkPhysicalDevice physical_device, uint32_t* pPropertyCount, VkExtensionProperties* pProperties) {
  return vkEnumerateDeviceExtensionProperties(physical_device, NULL, pPropertyCount, pProperties);
}

template<class InputIt, class C, class UnaryPredicate>
InputIt c_find_if(const C& container, const UnaryPredicate& p) {
  return find_if(container.begin(), container.end(), p);
}

bool DeviceSupportsExtensions(VkPhysicalDevice physical_device, const std::vector<const char*>& necessary_extensions) {
  auto supported_extensions = GetProps(physical_device, &DefaultDeviceExtensionProperties);

  std::set<std::string> required_extensions(necessary_extensions.begin(), necessary_extensions.end());
  for(const auto& extension : supported_extensions) {
    required_extensions.erase(extension.extensionName);
  }
  return required_extensions.empty();
}

bool DeviceSupportsSwapchain(VkPhysicalDevice physical_device, VkSurfaceKHR surface) {
  auto capabilities = vkh::GetPhysicalDeviceSurfaceCapabilitiesKHR(physical_device, surface);
  auto surface_formats = GetProps(physical_device, surface, &vkGetPhysicalDeviceSurfaceFormatsKHR);
  auto present_modes = GetProps(physical_device, surface, &vkGetPhysicalDeviceSurfacePresentModesKHR);
  return true;
}

// Asserts that the chosen physical device has support for the given surface.
VkPhysicalDevice ChoosePhysicalDevice(VkInstance instance, VkSurfaceKHR surface, const std::vector<const char*>& necessary_extensions) {
  auto physical_devices = GetProps(instance, &vkEnumeratePhysicalDevices);

  VkPhysicalDevice chosen_device = nullptr;
  for (uint32_t i = 0; i < physical_devices.size(); ++i) {
    VkPhysicalDevice device = physical_devices[i];
    VkPhysicalDeviceProperties properties = vkh::GetPhysicalDeviceProperties(device);

    std::cout << "Physical device: " << i << std::endl;
    if (GetQueueFamilySupportingSurface(device, surface) != -1 &&
        DeviceSupportsExtensions(device, necessary_extensions)) {
      chosen_device = device;
      if (properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
        return device;
      }
    }
  }

  assert(chosen_device != nullptr);
  return chosen_device;
}

// Returns -1 if no queue family has support.
int32_t GetQueueFamily(VkPhysicalDevice physical_device, VkQueueFlags flags) {
  auto family_properties = GetProps(physical_device, &vkGetPhysicalDeviceQueueFamilyProperties);

  for(int32_t i = 0; i < family_properties.size(); ++i) {
    if(family_properties[i].queueFlags & flags == flags) {
      return i;
    }
  }

  // No queue family supports these flags
  return -1;
}


int main() {
  SDL_Init(SDL_INIT_EVERYTHING);
  SDL_Window *window =
      SDL_CreateWindow("Affinity", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                       1024, 768, SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);

  vkh::ApplicationInfo F(app_info,
      pApplicationName = "Affinity",
      applicationVersion = 1,
      pEngineName = "Ocelot Engine",
      engineVersion = 1,
      apiVersion = VK_API_VERSION_1_1
  );

  const int layer_count = 1;
  const char *layer_names[] = {"VK_LAYER_LUNARG_standard_validation"};

  const int extension_count = 2;
  const char *extension_names[] = {VK_EXT_DEBUG_REPORT_EXTENSION_NAME,
                                   VK_KHR_SURFACE_EXTENSION_NAME};

  vkh::InstanceCreateInfo F(instance_info,
      pApplicationInfo = &app_info,
      enabledLayerCount = layer_count,
      ppEnabledLayerNames = layer_names,
      enabledExtensionCount = extension_count,
      ppEnabledExtensionNames = extension_names
  );

  VkInstance instance = vkh::CreateInstance(instance_info);

  VkSurfaceKHR surface;
  assert(SDL_Vulkan_CreateSurface(window, instance, &surface));

  const std::vector<const char*> device_extensions = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };
  VkPhysicalDevice physical_device = ChoosePhysicalDevice(instance, surface, device_extensions);

  int32_t graphics_queue_family = GetQueueFamily(physical_device, VK_QUEUE_GRAPHICS_BIT);
  int32_t transfer_queue_family = GetQueueFamily(physical_device, VK_QUEUE_TRANSFER_BIT);
  int32_t present_queue_family  = GetQueueFamilySupportingSurface(physical_device, surface);
  std::set<int32_t> queue_families = {graphics_queue_family, transfer_queue_family, present_queue_family};
  assert(graphics_queue_family != -1);
  assert(transfer_queue_family != -1);
  assert(present_queue_family != -1);

  std::vector<VkDeviceQueueCreateInfo> queue_create_infos;
  for(int32_t queue_family : {graphics_queue_family, transfer_queue_family, present_queue_family}) {
    vkh::DeviceQueueCreateInfo F(create_info,
        queueFamilyIndex = queue_family,
        queueCount = 1
    );
    queue_create_infos.push_back(create_info);
  }


  vkh::DeviceCreateInfo F(device_info,
      queueCreateInfoCount = queue_create_infos.size(),
      pQueueCreateInfos = queue_create_infos.data()
  );

  VkDevice device = vkh::CreateDevice(physical_device, device_info);

  VkQueue graphics_queue = vkh::GetDeviceQueue(device, graphics_queue_family, 0);
  VkQueue transfer_queue = vkh::GetDeviceQueue(device, transfer_queue_family, 0);
  VkQueue present_queue = vkh::GetDeviceQueue(device, present_queue_family, 0);

  vkDestroyDevice(device, nullptr);
  vkDestroySurfaceKHR(instance, surface, nullptr);
  vkDestroyInstance(instance, nullptr);
  return 0;
}
