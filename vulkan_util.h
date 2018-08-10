#include <cassert>
#include <cstring>
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

#define GET_MACRO(var, _1, _2, _3, _4, _5, _6, _7, _8, NAME, ...) NAME
#define F(...) GET_MACRO(__VA_ARGS__, FILL8, FILL7, FILL6, FILL5, FILL4, FILL3, FILL2, FILL1)(__VA_ARGS__)

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

#define FILL8(var, assign1, assign2, assign3, assign4, assign5, assign6, assign7, assign8) \
  FILL7(var, assign1, assign2, assign3, assign4, assign5, assign6, assign7); \
  var.assign8;


#define DVST(name, caps_name) \
namespace dv { \
struct name : public Vk ## name { \
  name() { \
    memset(this, 0, sizeof(Vk ## name)); \
    sType = VK_STRUCTURE_TYPE_ ## caps_name ; \
  } \
}; \
} /* namespace dv */ \
struct name : public dv :: name


#define DV(name) \
namespace dv { \
struct name : public Vk ## name { \
  name() { \
    memset(this, 0, sizeof(Vk ## name )); \
  } \
}; \
} /* namespace dv */ \
struct name : public dv :: name


// Generates a generic create helper function with the default behaviour of
// asserting success and returning the created object, while using no custom
// memory allocator.
#define DCE(type, extension) CE(type, extension) \
Vk ## type ## extension Create ## type ## extension (VkDevice device, const Vk ## type ## CreateInfo ## extension& create_info) { \
  Vk ## type ## extension type; \
  assert(vkCreate ## type ## extension(device, &create_info, nullptr, &type) == VK_SUCCESS); \
  return type; \
}

// Like DCE, but uses the global device
#define CE(type, extension) Vk ## type ## extension Create ## type ## extension (const Vk ## type ## CreateInfo ## extension& create_info) { \
  Vk ## type ## extension type; \
  assert(vkCreate ## type ## extension(device, &create_info, nullptr, &type) == VK_SUCCESS); \
  return type; \
}

// Generates a helper for a type that is core vulkan.
#define DC(type) DCE(type, )

namespace vkh {

VkPhysicalDevice physical_device;
VkDevice device;

DVST(ApplicationInfo, APPLICATION_INFO) {};

DVST(InstanceCreateInfo, INSTANCE_CREATE_INFO) {};

DVST(DeviceQueueCreateInfo, DEVICE_QUEUE_CREATE_INFO) {
  const std::vector<float> queue_priorities;
  DeviceQueueCreateInfo(uint32_t queue_count): queue_priorities(queue_count, 1.0) {
    queueCount = queue_count;
    pQueuePriorities = queue_priorities.data();
  }
};

DVST(DeviceCreateInfo, DEVICE_CREATE_INFO) {};

DVST(SwapchainCreateInfoKHR, SWAPCHAIN_CREATE_INFO_KHR) {
  SwapchainCreateInfoKHR() {
    imageArrayLayers = 1;
    preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
    compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    clipped = VK_TRUE;
  }
};

DV(ImageSubresourceRange) {
  ImageSubresourceRange(VkImageAspectFlags aspect_mask) {
    aspectMask = aspect_mask;
    levelCount = VK_REMAINING_MIP_LEVELS;
    layerCount = VK_REMAINING_ARRAY_LAYERS;
  }
};

DVST(ImageViewCreateInfo, IMAGE_VIEW_CREATE_INFO) {
  ImageViewCreateInfo() {
    viewType = VK_IMAGE_VIEW_TYPE_2D;
    subresourceRange = vkh::ImageSubresourceRange(VK_IMAGE_ASPECT_COLOR_BIT);
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
    vertexBindingDescriptionCount = 0;
    pVertexBindingDescriptions = nullptr;
    vertexAttributeDescriptionCount = 0;
    pVertexAttributeDescriptions = nullptr;
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

DVST(PipelineLayoutCreateInfo, PIPELINE_LAYOUT_CREATE_INFO) {};

const vkh::RasterizationState kRasterization;
const vkh::MultisampleState kNoMultisample;
const vkh::DepthStencilState kNoDepthTest;
const vkh::ColorBlendState kNoBlendingSingleAttachment;

DVST(GraphicsPipelineCreateInfo, GRAPHICS_PIPELINE_CREATE_INFO) {
  GraphicsPipelineCreateInfo() {
    pRasterizationState = &kRasterization;
    pMultisampleState = &kNoMultisample;
    pDepthStencilState = &kNoDepthTest;
    pColorBlendState = &kNoBlendingSingleAttachment;
  }
};

DVST(ShaderModuleCreateInfo, SHADER_MODULE_CREATE_INFO) {
  // We don't know if we'll use this source right away.
  const std::vector<char> source;
  ShaderModuleCreateInfo(const std::vector<char>& source): source(source) {
    codeSize = source.size();
    pCode = reinterpret_cast<const uint32_t*>(source.data());
  }
};

DC(ShaderModule);

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

DV(SubpassDescription) {
  SubpassDescription() {
    pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
  }
};

DVST(RenderPassCreateInfo, RENDER_PASS_CREATE_INFO) {};
DC(RenderPass);

std::string kMain = "main";
DVST(PipelineShaderStageCreateInfo, PIPELINE_SHADER_STAGE_CREATE_INFO) {
  PipelineShaderStageCreateInfo() {
    pName = kMain.c_str();
  }
};

DVST(FramebufferCreateInfo, FRAMEBUFFER_CREATE_INFO) {
  FramebufferCreateInfo() {
    layers = 1;
  }
};
DC(Framebuffer);

DVST(CommandPoolCreateInfo, COMMAND_POOL_CREATE_INFO) {
  CommandPoolCreateInfo(uint32_t queue_family_index) {
    queueFamilyIndex = queue_family_index;
  }
};
DC(CommandPool);

DVST(CommandBufferAllocateInfo, COMMAND_BUFFER_ALLOCATE_INFO) {
  CommandBufferAllocateInfo(VkCommandPool pool, uint32_t command_buffer_count) {
    commandPool = pool;
    commandBufferCount = command_buffer_count;
  }
};

DVST(CommandBufferBeginInfo, COMMAND_BUFFER_BEGIN_INFO) {
  CommandBufferBeginInfo() {
    flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
  }
};

DVST(SemaphoreCreateInfo, SEMAPHORE_CREATE_INFO) {};
DC(Semaphore);
VkSemaphore CreateSemaphore(VkDevice device) {
  return CreateSemaphore(device, SemaphoreCreateInfo());
}

DVST(FenceCreateInfo, FENCE_CREATE_INFO) {};
DC(Fence);
VkFence CreateFence(VkDevice device) {
  FenceCreateInfo create_info;
  create_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;
  return CreateFence(device, create_info);
}

DVST(SubmitInfo, SUBMIT_INFO) {};

DV(SubpassDependency) {
  SubpassDependency() {
    dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;
  }
};

DVST(PresentInfoKHR, PRESENT_INFO_KHR) {
  PresentInfoKHR(VkSemaphore* wait_semaphore, VkSwapchainKHR* swapchain, uint32_t* image_index) {
    waitSemaphoreCount = 1;
    pWaitSemaphores = wait_semaphore;

    swapchainCount = 1;
    pSwapchains = swapchain;
    pImageIndices = image_index;
  }
};

DVST(MemoryAllocateInfo, MEMORY_ALLOCATE_INFO) {};

void AllocateMemory(VkMemoryPropertyFlags required_memory_properties, const VkMemoryRequirements& memory_requirements, VkDeviceMemory* device_memory) {
  VkPhysicalDeviceMemoryProperties memory_properties;
  vkGetPhysicalDeviceMemoryProperties(physical_device, &memory_properties);

  int32_t found_memory = -1;
  for (uint32_t bit=0; bit < memory_properties.memoryTypeCount; ++bit) {
    if (memory_requirements.memoryTypeBits & (1 << bit) &&
        (memory_properties.memoryTypes[bit].propertyFlags &
            required_memory_properties) == required_memory_properties) {
      found_memory = bit;
    }
  }
  assert(found_memory != -1);

  vkh::MemoryAllocateInfo F(allocate_info,
      allocationSize = memory_requirements.size,
      memoryTypeIndex = found_memory
  );
  assert(vkAllocateMemory(device, &allocate_info, nullptr, device_memory) == VK_SUCCESS);
}

DVST(BufferCreateInfo, BUFFER_CREATE_INFO) {};
DC(Buffer);
VkBuffer CreateBuffer(VkDeviceSize buffer_size, VkBufferUsageFlags buffer_usage, VkMemoryPropertyFlags memory_properties, VkDeviceMemory* buffer_memory) {
  BufferCreateInfo F(buffer_info,
      size = buffer_size,
      usage = buffer_usage
  );
  auto buffer = CreateBuffer(device, buffer_info);

  VkMemoryRequirements memory_requirements;
  vkGetBufferMemoryRequirements(device, buffer, &memory_requirements);
  vkh::AllocateMemory(memory_properties, memory_requirements, buffer_memory);
  vkBindBufferMemory(device, buffer, *buffer_memory, 0);
  return buffer;
}

DVST(ImageCreateInfo, IMAGE_CREATE_INFO) {
  ImageCreateInfo() {
    imageType = VK_IMAGE_TYPE_2D;
    extent = {0, 0, 1};
    mipLevels = 1;
    arrayLayers = 1;
    initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    samples = VK_SAMPLE_COUNT_1_BIT;
  }
};
DC(Image);
VkImage CreateImage(uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags memory_properties, VkDeviceMemory* image_memory) {
  vkh::ImageCreateInfo F(image_info,
      extent.width = width,
      extent.height = height,
      format = format,
      tiling = tiling,
      usage = usage
  );
  auto image = CreateImage(image_info);

  VkMemoryRequirements memory_requirements;
  vkGetImageMemoryRequirements(device, image, &memory_requirements);
  vkh::AllocateMemory(memory_properties, memory_requirements, image_memory);
  vkBindImageMemory(device, image, *image_memory, 0);
  return image;
}

VkResult PresentQueue(VkQueue present_queue, VkSemaphore* wait_semaphore, VkSwapchainKHR* swapchain, uint32_t* image_index) {
  PresentInfoKHR present_info(wait_semaphore, swapchain, image_index);
  return vkQueuePresentKHR(present_queue, &present_info);
}


const VkClearValue kClearColor = {0, 0, 0, 1};
DVST(RenderPassBeginInfo, RENDER_PASS_BEGIN_INFO) {
  RenderPassBeginInfo(VkRenderPass render_pass, VkFramebuffer framebuffer_in, VkExtent2D extent) {
    renderPass = render_pass;
    framebuffer = framebuffer_in;
    renderArea.extent = extent;
    clearValueCount = 1;
    pClearValues = &kClearColor;
  }
};

DC(PipelineLayout);
DC(ImageView);
DCE(Swapchain, KHR);

// The following create functions don't follow the above patterns very well.
VkPipeline CreateGraphicsPipeline(VkDevice device, const VkGraphicsPipelineCreateInfo& create_info) {
  VkPipeline pipeline;
  assert(vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &create_info, nullptr, &pipeline) == VK_SUCCESS);
  return pipeline;
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

namespace h {
VkShaderModule ShaderModule(VkDevice device, const std::string& filename) {
  const auto source = ReadFile(filename);
  const auto create_info = vkh::ShaderModuleCreateInfo(source);
  return CreateShaderModule(device, create_info);
}
}
