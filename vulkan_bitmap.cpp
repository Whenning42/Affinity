#include "vulkan_util.h"

#include <algorithm>
#include <cassert>
#include <iostream>
#include <limits>
#include <set>
#include <vector>
#include <vulkan/vulkan.h>

#include <SDL2/SDL.h>
#include <SDL2/SDL_vulkan.h>

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
  return !surface_formats.empty() && !present_modes.empty();
}

VkSurfaceFormatKHR ChooseSwapchainSurfaceFormat(VkPhysicalDevice physical_device, VkSurfaceKHR surface) {
  auto surface_formats = GetProps(physical_device, surface, &vkGetPhysicalDeviceSurfaceFormatsKHR);
  auto preferred_format = VK_FORMAT_B8G8R8A8_UNORM;
  auto preferred_space = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;

  // The swapchain can use any format.
  if(surface_formats.size() == 1 && surface_formats[0].format == VK_FORMAT_UNDEFINED) {
    return {preferred_format, preferred_space};
  }

  for(const auto& format : surface_formats) {
    if(format.format == preferred_format && format.colorSpace == preferred_space) {
      return format;
    }
  }

  return surface_formats[0];
}

VkPresentModeKHR ChooseSwapchainPresentMode(VkPhysicalDevice physical_device, VkSurfaceKHR surface) {
  return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D ChooseSwapchainExtent(VkPhysicalDevice physical_device, VkSurfaceKHR surface) {
  auto capabilities = vkh::GetPhysicalDeviceSurfaceCapabilitiesKHR(physical_device, surface);

  if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
    return capabilities.currentExtent;
  } else {
    VkExtent2D actual_extent = {0, 0};

    actual_extent.width = std::max(capabilities.minImageExtent.width, std::min(capabilities.maxImageExtent.width, actual_extent.width));
    actual_extent.height = std::max(capabilities.minImageExtent.height, std::min(capabilities.maxImageExtent.height, actual_extent.height));
    return actual_extent;
  }
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
        DeviceSupportsExtensions(device, necessary_extensions) &&
        DeviceSupportsSwapchain(device, surface)) {
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
                       1024, 768, SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE | SDL_WINDOW_VULKAN);

  vkh::ApplicationInfo F(app_info,
      pApplicationName = "Affinity",
      applicationVersion = 1,
      pEngineName = "Ocelot Engine",
      engineVersion = 1,
      apiVersion = VK_API_VERSION_1_1
  );

  uint32_t sdl_extension_count = 0;
  // If this fails, vulkan is unsupported;
  assert(SDL_Vulkan_GetInstanceExtensions(window, &sdl_extension_count, NULL));

  std::vector<const char*> sdl_extensions(sdl_extension_count);
  assert(SDL_Vulkan_GetInstanceExtensions(window, &sdl_extension_count, sdl_extensions.data()));

  const int layer_count = 1;
  const char *layer_names[] = {"VK_LAYER_LUNARG_standard_validation"};

  const int extension_count = 2;
  const char *extension_names[] = {VK_EXT_DEBUG_REPORT_EXTENSION_NAME,
                                   VK_KHR_SURFACE_EXTENSION_NAME};

  const int combined_extension_count = extension_count + sdl_extension_count;
  const char *combined_extension_names[combined_extension_count];

  for(int32_t i=0; i<sdl_extension_count; ++i) {
    combined_extension_names[i] = sdl_extensions[i];
  }
  for(int32_t i=0; i<extension_count; ++i) {
    combined_extension_names[i + sdl_extension_count] = extension_names[i];
  }

  vkh::InstanceCreateInfo F(instance_info,
      pApplicationInfo = &app_info,
      enabledLayerCount = layer_count,
      ppEnabledLayerNames = layer_names,
      enabledExtensionCount = combined_extension_count,
      ppEnabledExtensionNames = combined_extension_names
  );

  VkInstance instance = vkh::CreateInstance(instance_info);

  VkDebugReportCallbackCreateInfoEXT create_info = {};
  create_info.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CALLBACK_CREATE_INFO_EXT;
  create_info.flags = VK_DEBUG_REPORT_ERROR_BIT_EXT | VK_DEBUG_REPORT_WARNING_BIT_EXT;
  create_info.pfnCallback = DebugCallback;

  VkDebugReportCallbackEXT callback;
  assert(CreateDebugReportCallbackEXT(instance, &create_info, nullptr, &callback) == VK_SUCCESS);

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
  for(int32_t queue_family : queue_families) {
    vkh::DeviceQueueCreateInfo create_info(1);
    queue_create_infos.push_back(create_info);
  }

  vkh::DeviceCreateInfo F(device_info,
      queueCreateInfoCount = queue_create_infos.size(),
      pQueueCreateInfos = queue_create_infos.data(),
      enabledExtensionCount = device_extensions.size(),
      ppEnabledExtensionNames = device_extensions.data()
  );
  auto device = vkh::CreateDevice(physical_device, device_info);

  VkQueue graphics_queue = vkh::GetDeviceQueue(device, graphics_queue_family, 0);
  VkQueue transfer_queue = vkh::GetDeviceQueue(device, transfer_queue_family, 0);
  VkQueue present_queue = vkh::GetDeviceQueue(device, present_queue_family, 0);

  auto swapchain_capabilities = vkh::GetPhysicalDeviceSurfaceCapabilitiesKHR(physical_device, surface);
  uint32_t image_count = swapchain_capabilities.minImageCount + 1;
  if (swapchain_capabilities.minImageCount == swapchain_capabilities.maxImageCount) {
    image_count = swapchain_capabilities.maxImageCount;
  }

  auto surface_format = ChooseSwapchainSurfaceFormat(physical_device, surface);
  auto extent = ChooseSwapchainExtent(physical_device, surface);

  // We'd expect to possibly change imageUsage, maybe queue families?
  vkh::SwapchainCreateInfoKHR F(swapchain_info,
      surface = surface,
      minImageCount = image_count,
      imageFormat = surface_format.format,
      imageColorSpace = surface_format.colorSpace,
      imageExtent = extent,
      imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
      presentMode = ChooseSwapchainPresentMode(physical_device, surface)
  );

  int32_t swapchain_families[] = {graphics_queue_family, present_queue_family};

  if (graphics_queue_family != present_queue_family) {
      swapchain_info.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
      swapchain_info.queueFamilyIndexCount = 2;
      swapchain_info.pQueueFamilyIndices = (uint32_t*)swapchain_families;
  } else {
      swapchain_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
  }

  VkSwapchainKHR swapchain = vkh::CreateSwapchainKHR(device, swapchain_info);

  auto swapchain_images = GetProps(device, swapchain, &vkGetSwapchainImagesKHR);
  std::vector<VkImageView> swapchain_image_views;
  for(const auto image : swapchain_images) {
    vkh::ImageViewCreateInfo F(image_view_info,
        image = image,
        format = surface_format.format
    );

    swapchain_image_views.push_back(vkh::CreateImageView(device, image_view_info));
  }

  // Graphics pipeline
  auto vert_source = ReadFile("shaders/quad.vert.spv");
  auto frag_source = ReadFile("shaders/quad.frag.spv");

  VkShaderModule vertex_module = vkh::ShaderModule::Create(device, vert_source);
  VkShaderModule fragment_module = vkh::ShaderModule::Create(device, frag_source);

  vkh::PipelineShaderStageCreateInfo F(vertex_stage_info,
     stage = VK_SHADER_STAGE_VERTEX_BIT,
     module = vertex_module
  );
  vkh::PipelineShaderStageCreateInfo F(fragment_stage_info,
     stage = VK_SHADER_STAGE_FRAGMENT_BIT,
     module = fragment_module
  );
  std::vector<VkPipelineShaderStageCreateInfo> pipeline_stages {vertex_stage_info, fragment_stage_info};
  const VkVertexInputBindingDescription kVertexBindingDescription{0, sizeof(float)*3};
  const VkVertexInputAttributeDescription kVertexAttributeDescription{0, 0, VK_FORMAT_R32G32B32_SFLOAT, 0};
  vkh::VertexInputState F(vertex_input_state,
     vertexBindingDescriptionCount = 1,
     pVertexBindingDescriptions = &kVertexBindingDescription,
     vertexAttributeDescriptionCount = 1,
     pVertexAttributeDescriptions = &kVertexAttributeDescription
  );

  vkh::InputAssemblyState input_assembly_state(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP);
  vkh::ViewportState viewport_state(extent);

  // Will be updated to add a descriptor for our bitmap uniform sampler
  vkh::PipelineLayoutCreateInfo F(pipeline_layout_info,
     setLayoutCount = 0,
     pSetLayouts = nullptr
  );
  auto pipeline_layout = vkh::CreatePipelineLayout(device, pipeline_layout_info);

  vkh::AttachmentDescription F(presentable_color_attachment,
     format = surface_format.format,
     finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR
  );

  VkAttachmentReference F(color_reference,
     attachment = 0,
     layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
  );

  vkh::SubpassDescription F(subpass,
     colorAttachmentCount = 1,
     pColorAttachments = &color_reference
  );

  vkh::RenderPassCreateInfo F(render_pass_info,
     attachmentCount = 1,
     pAttachments = &presentable_color_attachment,
     subpassCount = 1,
     pSubpasses = &subpass
  );
  auto render_pass = vkh::CreateRenderPass(device, render_pass_info);

  vkh::GraphicsPipelineCreateInfo F(pipeline_info,
     pStages = pipeline_stages.data(),
     pVertexInputState = &vertex_input_state,
     pInputAssemblyState = &input_assembly_state,
     pViewportState = &viewport_state,
     layout = pipeline_layout,
     renderPass = render_pass,
     subpass = 0
  );
  auto graphics_pipeline = vkh::CreateGraphicsPipeline(device, pipeline_info);

  std::vector<VkFramebuffer> swapchain_framebuffers(swapchain_image_views.size());
  for(auto& image_view : swapchain_image_views) {
    VkImageView* pAttachment = &image_view;

    vkh::FramebufferCreateInfo F(framebuffer_info,
        renderPass = render_pass,
        attachmentCount = 1,
        pAttachments = pAttachment
    );

    swapchain_framebuffers.push_back(vkh::CreateFramebuffer(device, framebuffer_info));
  };

  DestroyDebugReportCallbackEXT(instance, callback, nullptr);
  vkDestroyShaderModule(device, vertex_module, nullptr);
  vkDestroyShaderModule(device, fragment_module, nullptr);
  vkDestroyPipeline(device, graphics_pipeline, nullptr);
  vkDestroyPipelineLayout(device, pipeline_layout, nullptr);
  vkDestroyRenderPass(device, render_pass, nullptr);
  for(auto image_view : swapchain_image_views) {
    vkDestroyImageView(device, image_view, nullptr);
  }
  vkDestroySwapchainKHR(device, swapchain, nullptr);
  vkDestroyDevice(device, nullptr);
  vkDestroySurfaceKHR(instance, surface, nullptr);
  vkDestroyInstance(instance, nullptr);
  return 0;
}
