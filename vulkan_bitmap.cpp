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

#include <unistd.h>

#define MAX_IN_FLIGHT_FRAMES 2
uint32_t current_frame = 0;

const uint32_t kDefaultWidth = 1920;
const uint32_t kDefaultHeight = 1440;

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

VkExtent2D ChooseSwapchainExtent(VkPhysicalDevice physical_device, VkSurfaceKHR surface, SDL_Window* window) {
  auto capabilities = vkh::GetPhysicalDeviceSurfaceCapabilitiesKHR(physical_device, surface);

  if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
    return capabilities.currentExtent;
  } else {
    int32_t width, height;
    SDL_GetWindowSize(window, &width, &height);
    return {(uint32_t)width, (uint32_t)height};
  }
}

// Asserts that the chosen physical device has support for the given surface.
VkPhysicalDevice ChoosePhysicalDevice(VkInstance instance, VkSurfaceKHR surface, const std::vector<const char*>& necessary_extensions) {
  auto physical_devices = GetProps(instance, &vkEnumeratePhysicalDevices);

  VkPhysicalDevice chosen_device = nullptr;
  for (uint32_t i = 0; i < physical_devices.size(); ++i) {
    VkPhysicalDevice device = physical_devices[i];
    VkPhysicalDeviceProperties properties = vkh::GetPhysicalDeviceProperties(device);

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



class BitmapRenderer {
  VkPhysicalDevice physical_device;
  VkDevice device;
  std::vector<VkFramebuffer> swapchain_framebuffers;
  VkCommandPool command_pool;
  std::vector<VkCommandBuffer> command_buffers;
  VkPipeline graphics_pipeline;
  VkPipelineLayout pipeline_layout;
  VkRenderPass render_pass;
  std::vector<VkImageView> swapchain_image_views;
  VkSwapchainKHR swapchain;

  VkShaderModule vertex_module;
  VkShaderModule fragment_module;

  VkExtent2D swapchain_extent;
  VkSurfaceKHR surface;

  VkQueue graphics_queue;

  int32_t graphics_queue_family;
  int32_t present_queue_family;

  SDL_Window* window;

  void DestroySwapchain() {
    vkQueueWaitIdle(graphics_queue);

    for (size_t i = 0; i < swapchain_framebuffers.size(); i++) {
        vkDestroyFramebuffer(device, swapchain_framebuffers[i], nullptr);
    }
    swapchain_framebuffers.clear();

    vkFreeCommandBuffers(device, command_pool, static_cast<uint32_t>(command_buffers.size()), command_buffers.data());

    vkDestroyPipeline(device, graphics_pipeline, nullptr);
    vkDestroyPipelineLayout(device, pipeline_layout, nullptr);
    vkDestroyRenderPass(device, render_pass, nullptr);

    for (size_t i = 0; i < swapchain_image_views.size(); i++) {
        vkDestroyImageView(device, swapchain_image_views[i], nullptr);
    }
    swapchain_image_views.clear();

    vkDestroySwapchainKHR(device, swapchain, nullptr);
  }

  void RecreateSwapchain() {
    auto swapchain_capabilities = vkh::GetPhysicalDeviceSurfaceCapabilitiesKHR(physical_device, surface);
    uint32_t image_count = swapchain_capabilities.minImageCount + 1;
    if (swapchain_capabilities.minImageCount == swapchain_capabilities.maxImageCount) {
      image_count = swapchain_capabilities.maxImageCount;
    }

    auto surface_format = ChooseSwapchainSurfaceFormat(physical_device, surface);
    swapchain_extent = ChooseSwapchainExtent(physical_device, surface, window);

    // We'd expect to possibly change imageUsage, maybe queue families?
    vkh::SwapchainCreateInfoKHR F(swapchain_info,
        surface = surface,
        minImageCount = image_count,
        imageFormat = surface_format.format,
        imageColorSpace = surface_format.colorSpace,
        imageExtent = swapchain_extent,
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

    swapchain = vkh::CreateSwapchainKHR(swapchain_info);

    auto swapchain_images = GetProps(device, swapchain, &vkGetSwapchainImagesKHR);
    for(const auto image : swapchain_images) {
      vkh::ImageViewCreateInfo F(image_view_info,
          image = image,
          format = surface_format.format
      );

      swapchain_image_views.push_back(vkh::CreateImageView(image_view_info));
    }

    // Graphics pipeline create start


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
    /*
    vkh::VertexInputState F(vertex_input_state,
       vertexBindingDescriptionCount = 1,
       pVertexBindingDescriptions = &kVertexBindingDescription,
       vertexAttributeDescriptionCount = 1,
       pVertexAttributeDescriptions = &kVertexAttributeDescription
    );*/
    vkh::VertexInputState vertex_input_state;

    vkh::InputAssemblyState input_assembly_state(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP);
    vkh::ViewportState viewport_state(swapchain_extent);

    // Will be updated to add a descriptor for our bitmap uniform sampler
    vkh::PipelineLayoutCreateInfo F(pipeline_layout_info,
       setLayoutCount = 0,
       pSetLayouts = nullptr
    );
    pipeline_layout = vkh::CreatePipelineLayout(pipeline_layout_info);

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

    vkh::SubpassDependency F(subpass_dependency,
        srcSubpass = VK_SUBPASS_EXTERNAL,
        srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
        dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
        dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT
    );

    vkh::RenderPassCreateInfo F(render_pass_info,
       attachmentCount = 1,
       pAttachments = &presentable_color_attachment,
       subpassCount = 1,
       pSubpasses = &subpass,
       dependencyCount = 1,
       pDependencies = &subpass_dependency
    );
    render_pass = vkh::CreateRenderPass(render_pass_info);

    vkh::GraphicsPipelineCreateInfo F(pipeline_info,
       stageCount = pipeline_stages.size(),
       pStages = pipeline_stages.data(),
       pVertexInputState = &vertex_input_state,
       pInputAssemblyState = &input_assembly_state,
       pViewportState = &viewport_state,
       layout = pipeline_layout,
       renderPass = render_pass,
       subpass = 0
    );
    graphics_pipeline = vkh::CreateGraphicsPipeline(device, pipeline_info);

    for(auto& image_view : swapchain_image_views) {
      vkh::FramebufferCreateInfo F(framebuffer_info,
          renderPass = render_pass,
          attachmentCount = 1,
          pAttachments = &image_view,
          width = swapchain_extent.width,
          height = swapchain_extent.height
      );

      swapchain_framebuffers.push_back(vkh::CreateFramebuffer(framebuffer_info));
    };

    command_buffers.resize(swapchain_framebuffers.size());
    vkh::CommandBufferAllocateInfo command_buffer_allocate_info(command_pool, command_buffers.size());
    assert(vkAllocateCommandBuffers(device, &command_buffer_allocate_info, command_buffers.data()) == VK_SUCCESS);

    for (uint32_t i=0; i<swapchain_framebuffers.size(); ++i) {
      auto& command_buffer = command_buffers[i];
      vkh::CommandBufferBeginInfo begin_info;
      assert(vkBeginCommandBuffer(command_buffer, &begin_info) == VK_SUCCESS);

      vkh::RenderPassBeginInfo render_pass_begin_info(render_pass, swapchain_framebuffers[i], swapchain_extent);
      vkCmdBeginRenderPass(command_buffer, &render_pass_begin_info, VK_SUBPASS_CONTENTS_INLINE);

      vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphics_pipeline);
      vkCmdDraw(command_buffer, 4, 1, 0, 0);

      vkCmdEndRenderPass(command_buffer);
      assert(vkEndCommandBuffer(command_buffer) == VK_SUCCESS);
    }

  }

public:
  BitmapRenderer() {}

  void Run() {
    SDL_Init(SDL_INIT_EVERYTHING);
    window = SDL_CreateWindow(
        "Affinity", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        kDefaultWidth, kDefaultHeight,
        SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE | SDL_WINDOW_VULKAN);

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

    assert(SDL_Vulkan_CreateSurface(window, instance, &surface));

    const std::vector<const char*> device_extensions = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };
    physical_device = ChoosePhysicalDevice(instance, surface, device_extensions);
    vkh::physical_device = physical_device;

    graphics_queue_family = GetQueueFamily(physical_device, VK_QUEUE_GRAPHICS_BIT);
    int32_t transfer_queue_family = GetQueueFamily(physical_device, VK_QUEUE_TRANSFER_BIT);
    present_queue_family  = GetQueueFamilySupportingSurface(physical_device, surface);
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
    device = vkh::CreateDevice(physical_device, device_info);
    vkh::device = device;

    graphics_queue = vkh::GetDeviceQueue(device, graphics_queue_family, 0);
    VkQueue transfer_queue = vkh::GetDeviceQueue(device, transfer_queue_family, 0);
    VkQueue present_queue = vkh::GetDeviceQueue(device, present_queue_family, 0);


    vkh::CommandPoolCreateInfo command_pool_info(graphics_queue_family);
    command_pool = vkh::CreateCommandPool(command_pool_info);

    vertex_module = h::ShaderModule(device, "shaders/quad.vert.spv");
    fragment_module = h::ShaderModule(device, "shaders/quad.frag.spv");

    RecreateSwapchain();

    std::vector<VkSemaphore> image_available_semaphores;
    std::vector<VkSemaphore> render_finished_semaphores;
    std::vector<VkFence> in_flight_fences;

    for(uint32_t i=0; i<MAX_IN_FLIGHT_FRAMES; ++i) {
      image_available_semaphores.push_back(vkh::CreateSemaphore(device));
      render_finished_semaphores.push_back(vkh::CreateSemaphore(device));
      in_flight_fences.push_back(vkh::CreateFence(device));
    }

    VkPipelineStageFlags wait_stage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

    SDL_Event event;

    uint32_t kImageHeight = 512;
    uint32_t kImageWidth = 512;
    size_t texture_size = kImageHeight * kImageWidth * 4;
    VkDeviceMemory buffer_memory;

    auto staging_buffer = vkh::CreateBuffer(texture_size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &buffer_memory);

    void* texture_data;
    vkMapMemory(device, buffer_memory, 0, texture_size, 0, &texture_data);
    memset(texture_data, 128, texture_size);
    vkUnmapMemory(device, buffer_memory);

    VkDeviceMemory texture_memory;
    auto texture_image = vkh::CreateImage(kImageWidth, kImageHeight, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &texture_memory);

    bool run = true;
    while (run) {
      while(SDL_PollEvent(&event)) {
        if(event.type == SDL_QUIT) {
          run = false;
        }
      }

      current_frame = (current_frame + 1) % MAX_IN_FLIGHT_FRAMES;

      vkWaitForFences(device, 1, &in_flight_fences[current_frame], VK_TRUE, std::numeric_limits<uint64_t>::max());

      VkSemaphore& wait_semaphore = image_available_semaphores[current_frame];
      VkSemaphore& signal_semaphore = render_finished_semaphores[current_frame];

      uint32_t image_index;
      VkResult result = vkAcquireNextImageKHR(device, swapchain, std::numeric_limits<uint64_t>::max(), wait_semaphore, VK_NULL_HANDLE, &image_index);
      if(result == VK_ERROR_OUT_OF_DATE_KHR) {
        DestroySwapchain();
        RecreateSwapchain();
      } else {
        assert(result == VK_SUCCESS || result == VK_SUBOPTIMAL_KHR);
      }

      vkResetFences(device, 1, &in_flight_fences[current_frame]);

      vkh::SubmitInfo F(submit_info,
          waitSemaphoreCount = 1,
          pWaitSemaphores = &wait_semaphore,
          signalSemaphoreCount = 1,
          pSignalSemaphores = &signal_semaphore,
          pWaitDstStageMask = &wait_stage,
          commandBufferCount = 1,
          pCommandBuffers = &command_buffers[image_index]
      );

      assert(vkQueueSubmit(graphics_queue, 1, &submit_info, in_flight_fences[current_frame]) == VK_SUCCESS);
      result = vkh::PresentQueue(present_queue, &signal_semaphore, &swapchain, &image_index);
      if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
        DestroySwapchain();
        RecreateSwapchain();
      } else {
        assert(result == VK_SUCCESS);
      }

      SDL_Delay(1);
    }
    vkQueueWaitIdle(present_queue);


    for(uint32_t i=0; i<MAX_IN_FLIGHT_FRAMES; ++i) {
      vkDestroySemaphore(device, image_available_semaphores[i], nullptr);
      vkDestroySemaphore(device, render_finished_semaphores[i], nullptr);
      vkDestroyFence(device, in_flight_fences[i], nullptr);
    }
    DestroyDebugReportCallbackEXT(instance, callback, nullptr);
    vkDestroyShaderModule(device, vertex_module, nullptr);
    vkDestroyShaderModule(device, fragment_module, nullptr);
    DestroySwapchain();
    vkDestroyCommandPool(device, command_pool, nullptr);
    vkDestroyDevice(device, nullptr);
    vkDestroySurfaceKHR(instance, surface, nullptr);
    vkDestroyInstance(instance, nullptr);
  }
};

int main() {
  BitmapRenderer renderer;
  renderer.Run();
}
