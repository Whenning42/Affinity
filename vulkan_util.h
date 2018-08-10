#include <cassert>
#include <fstream>
#include <iostream>
#include <set>
#include <tuple>
#include <type_traits>
#include <vector>
#include <vulkan/vulkan.h>

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

#define GET_MACRO(var, _1, _2, _3, _4, _5, _6, _7, NAME, ...) NAME
#define F(...) GET_MACRO(__VA_ARGS__, FILL7, FILL6, FILL5, FILL4, FILL3, FILL2, FILL1)(__VA_ARGS__)

#define FILL1(var, assign1) \
  var; \
  var.assign1;

#define FILL2(var, assign1, assign2) \
  FILL1(var, assign1); \
  var.assign2;

#define FILL3(var, assign1, assign2, assign3) \
  FILL2(var, assign1, assign2); \
  var.assign3;

#define FILL4(var, assign1, assign2, assign3, assign4) \
  FILL3(var, assign1, assign2, assign3); \
  var.assign4;

#define FILL5(var, assign1, assign2, assign3, assign4, assign5) \
  FILL4(var, assign1, assign2, assign3, assign4); \
  var.assign5;

#define FILL6(var, assign1, assign2, assign3, assign4, assign5, assign6) \
  FILL5(var, assign1, assign2, assign3, assign4, assign5); \
  var.assign6;

#define FILL7(var, assign1, assign2, assign3, assign4, assign5, assign6, assign7) \
  FILL6(var, assign1, assign2, assign3, assign4, assign5, assign6); \
  var.assign7;

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
    flags = 0;
    enabledExtensionCount = 0;
    ppEnabledExtensionNames = nullptr;
    enabledLayerCount = 0;
    ppEnabledLayerNames = nullptr;
    pEnabledFeatures = nullptr;
  }
};

struct SwapchainCreateInfoKHR : public VkSwapchainCreateInfoKHR {
  SwapchainCreateInfoKHR() {
    sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    pNext = nullptr;
    flags = 0;

    // Have to set surface through pQueueFamilyIndices
    // with the exception of
    imageArrayLayers = 1;
    // And have to set presentMode

    queueFamilyIndexCount = 0;
    pQueueFamilyIndices = nullptr;
    preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
    compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    clipped = VK_TRUE;
    oldSwapchain = VK_NULL_HANDLE;
  }
};

struct ImageSubresourceRange : public VkImageSubresourceRange {
  ImageSubresourceRange(VkImageAspectFlags aspect_mask) {
    aspectMask = aspect_mask;
    baseMipLevel = 0;
    levelCount = VK_REMAINING_MIP_LEVELS;
    baseArrayLayer = 0;
    layerCount = VK_REMAINING_ARRAY_LAYERS;
  }
};

struct ImageViewCreateInfo : public VkImageViewCreateInfo {
  ImageViewCreateInfo() {
    sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    pNext = nullptr;
    flags = 0;
    viewType = VK_IMAGE_VIEW_TYPE_2D;
    components = {};
    subresourceRange = vkh::ImageSubresourceRange(VK_IMAGE_ASPECT_COLOR_BIT);

    // Need to set image and format
  }
};

using VkVertexInputState = VkPipelineVertexInputStateCreateInfo;
using VkInputAssemblyState = VkPipelineInputAssemblyStateCreateInfo;
using VkViewportState = VkPipelineViewportStateCreateInfo;
using VkRasterizationState = VkPipelineRasterizationStateCreateInfo;
using VkMultisampleState = VkPipelineMultisampleStateCreateInfo;
using VkDepthStencilState = VkPipelineDepthStencilStateCreateInfo;
using VkColorBlendState = VkPipelineColorBlendStateCreateInfo;

struct VertexInputState : public VkVertexInputState {
  VertexInputState() {
    sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    pNext = nullptr;
    flags = 0;
  }
};

struct InputAssemblyState : public VkInputAssemblyState {
  private:
  InputAssemblyState() {
    sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    pNext = nullptr;
    flags = 0;
    primitiveRestartEnable = VK_FALSE;
  }

  public:
  InputAssemblyState(VkPrimitiveTopology topology_in): InputAssemblyState() {
    topology = topology_in;
  }
};

struct Viewport : public VkViewport {
  Viewport(const VkExtent2D& extent) {
    x = 0;
    y = 0;
    width = extent.width;
    height = extent.height;
    minDepth = 0;
    maxDepth = 1;
  }
};

struct Scissor : public VkRect2D {
  Scissor(const VkExtent2D& extent_in) {
    offset = {0, 0};
    extent = extent_in;
  }
};

struct ViewportState : public VkViewportState {
  const Viewport viewport;
  const Scissor scissor;

  ViewportState(const VkExtent2D& extent): viewport(extent), scissor(extent) {
    sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    pNext = nullptr;
    flags = 0;
    viewportCount = 1;
    pViewports = &viewport;
    scissorCount = 1;
    pScissors = &scissor;
  }
};

struct RasterizationState : public VkRasterizationState {
  RasterizationState() {
    sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    pNext = nullptr;
    flags = 0;
    depthClampEnable = VK_FALSE;
    rasterizerDiscardEnable = VK_FALSE;
    polygonMode = VK_POLYGON_MODE_FILL;
    cullMode = VK_CULL_MODE_NONE;
    depthBiasEnable = VK_FALSE;
    lineWidth = 1.0;
  }
};

struct MultisampleState : public VkMultisampleState {
  MultisampleState() {
    sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    pNext = nullptr;
    flags = 0;
    rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    sampleShadingEnable = VK_FALSE;
  }
};

struct DepthStencilState : public VkDepthStencilState {
  DepthStencilState() {
    sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    pNext = nullptr;
    flags = 0;
    depthTestEnable = VK_FALSE;
    depthWriteEnable = VK_FALSE;
    depthBoundsTestEnable = VK_FALSE;
    stencilTestEnable = VK_FALSE;
  }
};

struct ColorBlendAttachmentState : public VkPipelineColorBlendAttachmentState {
  ColorBlendAttachmentState() {
    blendEnable = VK_FALSE;
    colorWriteMask = 0xf;
  }
};

const ColorBlendAttachmentState kNoBlendSingleAttachment;
struct ColorBlendState : public VkColorBlendState {
  ColorBlendState() {
    sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    pNext = nullptr;
    flags = 0;
    logicOpEnable = VK_FALSE;
    attachmentCount = 1;
    pAttachments = &kNoBlendSingleAttachment;
  }
};

struct PipelineLayoutCreateInfo : public VkPipelineLayoutCreateInfo {
  PipelineLayoutCreateInfo() {
    sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pNext = nullptr;
    flags = 0;
    pushConstantRangeCount = 0;
    pPushConstantRanges = nullptr;
  }
};

const vkh::RasterizationState kRasterization;
const vkh::MultisampleState kNoMultisample;
const vkh::DepthStencilState kNoDepthTest;
const vkh::ColorBlendState kNoBlendingSingleAttachment;
struct GraphicsPipelineCreateInfo : public VkGraphicsPipelineCreateInfo {
  GraphicsPipelineCreateInfo() {
    sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pNext = nullptr;
    flags = 0;
    stageCount = 2;
    //pStages
    //pVertexInputState
    //pVertexAsseblyState
    pTessellationState = nullptr;
    pRasterizationState = &kRasterization;
    pMultisampleState = &kNoMultisample;
    pDepthStencilState = &kNoDepthTest;
    pColorBlendState = &kNoBlendingSingleAttachment;
    pDynamicState = nullptr;
    renderPass = nullptr;
    basePipelineHandle = VK_NULL_HANDLE;
    basePipelineIndex = -1;
  }
};

struct ShaderModule {
  static VkShaderModule Create(VkDevice device, const std::vector<char>& source) {
    VkShaderModuleCreateInfo F(create_info,
      sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
      pNext = nullptr,
      flags = 0,
      codeSize = source.size(),
      pCode = reinterpret_cast<const uint32_t*>(source.data())
    );

    VkShaderModule shader_module;
    assert(vkCreateShaderModule(device, &create_info, nullptr, &shader_module) == VK_SUCCESS);
    return shader_module;
  }
};

struct AttachmentDescription : public VkAttachmentDescription {
  AttachmentDescription() {
    flags = 0;
    samples = VK_SAMPLE_COUNT_1_BIT;
    loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  }
};

struct AttachmentReference : public VkAttachmentReference {};

struct SubpassDescription : public VkSubpassDescription {
  SubpassDescription() {
    flags = 0;
    pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    inputAttachmentCount = 0;
    pInputAttachments = nullptr;
    colorAttachmentCount = 0;
    pColorAttachments = nullptr;
    pResolveAttachments = nullptr;
    pDepthStencilAttachment = nullptr;
    preserveAttachmentCount = 0;
    pPreserveAttachments = nullptr;
  }
};

struct RenderPassCreateInfo : public VkRenderPassCreateInfo {
  RenderPassCreateInfo() {
    sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    pNext = nullptr;
    flags = 0;
    attachmentCount = 0;
    pAttachments = nullptr;
    subpassCount = 0;
    pSubpasses = nullptr;
    dependencyCount = 0;
    pDependencies = nullptr;
  }
};

std::string kMain = "main";
struct PipelineShaderStageCreateInfo : public VkPipelineShaderStageCreateInfo {
  PipelineShaderStageCreateInfo() {
    sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    pNext = 0;
    flags = 0;
    module = nullptr;
    pName = kMain.c_str();
    pSpecializationInfo = nullptr;
  }
};

struct FramebufferCreateInfo : public VkFramebufferCreateInfo {
  FramebufferCreateInfo() {
    sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    pNext = 0;
    flags = 0;
  }
};

#define DC(type) Vk ## type Create ## type (VkDevice device, const Vk ## type ## CreateInfo& create_info) { \
  Vk ## type type; \
  assert(vkCreate ## type (device, &create_info, nullptr, &type) == VK_SUCCESS); \
  return type; \
}

DC(Framebuffer);
DC(RenderPass);
DC(PipelineLayout);
DC(ImageView);
/*
VkFramebuffer CreateFramebuffer(VkDevice device, const VkFramebufferCreateInfo& create_info) {
  VkFramebuffer framebuffer;
  assert(vkCreateFramebuffer(device, &create_info, nullptr, &framebuffer) == VK_SUCCESS);
  return framebuffer;
}*/

VkPipeline CreateGraphicsPipeline(VkDevice device, const VkGraphicsPipelineCreateInfo& create_info) {
  VkPipeline pipeline;
  assert(vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &create_info, nullptr, &pipeline) == VK_SUCCESS);
  return pipeline;
}
/*
VkRenderPass CreateRenderPass(VkDevice device, const VkRenderPassCreateInfo& create_info) {
  VkRenderPass render_pass;
  assert(vkCreateRenderPass(device, &create_info, nullptr, &render_pass) == VK_SUCCESS);
  return render_pass;
}*/

/*
VkPipelineLayout CreatePipelineLayout(VkDevice device, const VkPipelineLayoutCreateInfo& create_info) {
  VkPipelineLayout pipeline_layout;
  assert(vkCreatePipelineLayout(device, &create_info, nullptr, &pipeline_layout) == VK_SUCCESS);
  return pipeline_layout;
}*/

/*
VkImageView CreateImageView(VkDevice device, const VkImageViewCreateInfo& create_info) {
  VkImageView image_view;
  assert(vkCreateImageView(device, &create_info, nullptr, &image_view) == VK_SUCCESS);
  return image_view;
}*/

VkImageView CreateImageView(VkDevice device, VkImage image, VkImageViewType view_type, VkFormat format, VkImageAspectFlags aspect_mask) {

  vkh::ImageViewCreateInfo F(create_info,
      image = image,
      viewType = view_type,
      format = format,
      subresourceRange.aspectMask = aspect_mask
  );

  return CreateImageView(device, create_info);
}

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

VkSwapchainKHR CreateSwapchainKHR(VkDevice device, const VkSwapchainCreateInfoKHR& create_info) {
  VkSwapchainKHR swapchain;
  assert(vkCreateSwapchainKHR(device, &create_info, nullptr, &swapchain) == VK_SUCCESS);
  return swapchain;
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

std::vector<char> ReadFile(const std::string& filename) {
  std::ifstream file(filename);
  assert(file.is_open());

  file.seekg(0, std::ios::end);
  size_t length = file.tellg();
  file.seekg(0, std::ios::beg);

  std::vector<char> file_contents(length);
  file.read(file_contents.data(), length);
  return file_contents;
}

// Functions all come from demo.
static VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallback(
    VkDebugReportFlagsEXT flags,
    VkDebugReportObjectTypeEXT objType,
    uint64_t obj,
    size_t location,
    int32_t code,
    const char* layerPrefix,
    const char* msg,
    void* userData) {

    std::cerr << "validation layer: " << msg << std::endl;

    assert(false);
    return VK_FALSE;
}

VkResult CreateDebugReportCallbackEXT(VkInstance instance, const VkDebugReportCallbackCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugReportCallbackEXT* pCallback) {
    auto func = (PFN_vkCreateDebugReportCallbackEXT) vkGetInstanceProcAddr(instance, "vkCreateDebugReportCallbackEXT");
    if (func != nullptr) {
        return func(instance, pCreateInfo, pAllocator, pCallback);
    } else {
        return VK_ERROR_EXTENSION_NOT_PRESENT;
    }
}

void DestroyDebugReportCallbackEXT(VkInstance instance, VkDebugReportCallbackEXT callback, const VkAllocationCallbacks* pAllocator) {
    auto func = (PFN_vkDestroyDebugReportCallbackEXT) vkGetInstanceProcAddr(instance, "vkDestroyDebugReportCallbackEXT");
    if (func != nullptr) {
        func(instance, callback, pAllocator);
    }
}
