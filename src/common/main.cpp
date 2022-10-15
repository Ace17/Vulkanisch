#include <climits>
#include <cstdint>
#include <cstdio>
#include <map>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

#include "app.h"
#include "util.h"

///////////////////////////////////////////////////////////////////////////////
// app registry
auto& Registry()
{
  static std::map<std::string, AppCreationFunc> appRegistry;
  return appRegistry;
}

int registerApp(const char* name, AppCreationFunc creationFunc)
{
  printf("Registered: '%s'\n", name);
  Registry()[name] = creationFunc;
  return 0;
}

///////////////////////////////////////////////////////////////////////////////
// Vulkan includes
#include "glad/vulkan.h"

#include "SDL.h"
#include "SDL_vulkan.h"

///////////////////////////////////////////////////////////////////////////////

PFN_vkVoidFunction (*g_vkGetInstanceProcAddr)(VkInstance instance, const char* pName);

void loadVulkanUsingGlad(VkInstance instance, VkPhysicalDevice device)
{
  static auto GetVulkanFunction = [](void* userPtr, const char* name) {
    auto result = g_vkGetInstanceProcAddr((VkInstance)userPtr, name);
    typedef void (*ProcType)();
    return (ProcType)result;
  };

  if(!gladLoadVulkanUserPtr(device, GetVulkanFunction, instance))
    throw std::runtime_error("Vulkan isn't installed");
}

///////////////////////////////////////////////////////////////////////////////

const uint32_t WIDTH = 800;
const uint32_t HEIGHT = 600;

const char* const RequiredDeviceExtensions[] = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};

struct QueueFamilyIndices
{
  uint32_t graphicsFamily;
  uint32_t presentFamily;
  bool hasGraphicsFamily = false;
  bool hasPresentFamily = false;

  bool isComplete() const { return hasGraphicsFamily && hasPresentFamily; }
};

struct SwapChainSupportDetails
{
  VkSurfaceCapabilitiesKHR capabilities;
  std::vector<VkSurfaceFormatKHR> formats;
  std::vector<VkPresentModeKHR> presentModes;
};

void normalizeMatrix(Matrix4f& mat)
{
  Vec3f Column0 = {mat[0][0], mat[1][0], mat[2][0]};
  Vec3f Column1 = {mat[0][1], mat[1][1], mat[2][1]};
  Vec3f Column2 = {mat[0][2], mat[1][2], mat[2][2]};

  Column0 = ::normalize(crossProduct(Column1, Column2));
  Column1 = ::normalize(crossProduct(Column2, Column0));
  Column2 = ::normalize(Column2);

  mat[0][0] = Column0.x;
  mat[1][0] = Column0.y;
  mat[2][0] = Column0.z;

  mat[0][1] = Column1.x;
  mat[1][1] = Column1.y;
  mat[2][1] = Column1.z;

  mat[0][2] = Column2.x;
  mat[1][2] = Column2.y;
  mat[2][2] = Column2.z;
}

static VkImageView createImageView(VkDevice device, VkImage image, VkFormat format)
{
  VkImageViewCreateInfo createInfo{};
  createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
  createInfo.image = image;
  createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
  createInfo.format = format;
  createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
  createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
  createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
  createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
  createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  createInfo.subresourceRange.baseMipLevel = 0;
  createInfo.subresourceRange.levelCount = 1;
  createInfo.subresourceRange.baseArrayLayer = 0;
  createInfo.subresourceRange.layerCount = 1;

  VkImageView view;
  if(vkCreateImageView(device, &createInfo, nullptr, &view) != VK_SUCCESS)
    throw std::runtime_error("failed to create image views!");

  return view;
}

static std::vector<const char*> getRequiredExtensions()
{
  unsigned int count = 0;
  if(!SDL_Vulkan_GetInstanceExtensions(nullptr, &count, nullptr))
    throw std::runtime_error("Couldn't get SDL required Vulkan extensions");

  std::vector<const char*> result(count);
  if(!SDL_Vulkan_GetInstanceExtensions(nullptr, &count, result.data()))
    throw std::runtime_error("Couldn't get SDL required Vulkan extensions");

  for(auto& name : result)
    fprintf(stderr, "SDL requires: %s\n", name);

  return result;
}

static VkInstance createInstance()
{
  VkInstance instance;

  VkApplicationInfo appInfo{};
  appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
  appInfo.apiVersion = VK_MAKE_VERSION(1, 1, 0);

  VkInstanceCreateInfo createInfo{};
  createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
  createInfo.pApplicationInfo = &appInfo;

  auto extensions = getRequiredExtensions();
  createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
  createInfo.ppEnabledExtensionNames = extensions.data();

  if(vkCreateInstance(&createInfo, nullptr, &instance) != VK_SUCCESS)
  {
    throw std::runtime_error("failed to create instance!");
  }

  return instance;
}

static VkSurfaceKHR createSurface(VkInstance instance, SDL_Window* window)
{
  VkSurfaceKHR result;

  if(!SDL_Vulkan_CreateSurface(window, instance, &result))
    throw std::runtime_error("Unable to create Vulkan compatible surface using SDL");

  return result;
}

static QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device, VkSurfaceKHR surface)
{
  uint32_t queueFamilyCount = 0;
  vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

  std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
  vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

  QueueFamilyIndices indices;

  int i = 0;

  for(const auto& queueFamily : queueFamilies)
  {
    if(queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT)
    {
      indices.graphicsFamily = i;
      indices.hasGraphicsFamily = true;
    }

    VkBool32 presentSupport = false;
    vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentSupport);

    if(presentSupport)
    {
      indices.presentFamily = i;
      indices.hasPresentFamily = true;
    }

    if(indices.isComplete())
    {
      break;
    }

    i++;
  }

  return indices;
}

static bool checkDeviceExtensionSupport(VkPhysicalDevice device)
{
  uint32_t extensionCount;
  vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);

  std::vector<VkExtensionProperties> availableExtensions(extensionCount);
  vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());

  for(auto& reqExtension : RequiredDeviceExtensions)
  {
    const std::string name = reqExtension;
    bool found = false;

    for(auto& candidateExtension : availableExtensions)
    {
      if(name == candidateExtension.extensionName)
        found = true;
    }

    if(!found)
      return false;
  }

  return true;
}

static SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device, VkSurfaceKHR surface)
{
  SwapChainSupportDetails details;

  vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &details.capabilities);

  uint32_t formatCount;
  vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, nullptr);

  if(formatCount != 0)
  {
    details.formats.resize(formatCount);
    vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, details.formats.data());
  }

  uint32_t presentModeCount;
  vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, nullptr);

  if(presentModeCount != 0)
  {
    details.presentModes.resize(presentModeCount);
    vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, details.presentModes.data());
  }

  return details;
}

static bool isDeviceSuitable(VkPhysicalDevice device, VkSurfaceKHR surface)
{
  QueueFamilyIndices indices = findQueueFamilies(device, surface);

  if(!indices.isComplete())
    return false;

  if(!checkDeviceExtensionSupport(device))
    return false;

  const SwapChainSupportDetails details = querySwapChainSupport(device, surface);

  if(details.formats.empty() || details.presentModes.empty())
    return false;

  return true;
}

static VkPhysicalDevice pickPhysicalDevice(VkInstance instance, VkSurfaceKHR surface)
{
  uint32_t deviceCount = 0;
  vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);

  if(deviceCount == 0)
    throw std::runtime_error("failed to find a GPU with Vulkan support");

  VkPhysicalDevice physicalDevice;
  std::vector<VkPhysicalDevice> devices(deviceCount);
  vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());

  for(const auto& device : devices)
  {
    if(isDeviceSuitable(device, surface))
    {
      physicalDevice = device;
      break;
    }
  }

  if(physicalDevice == VK_NULL_HANDLE)
    throw std::runtime_error("failed to find a suitable GPU");

  return physicalDevice;
}

static VkDevice createLogicalDevice(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface, VkQueue* graphicsQueue, VkQueue* presentQueue)
{
  QueueFamilyIndices indices = findQueueFamilies(physicalDevice, surface);

  const float queuePriority = 1.0f;

  VkDeviceQueueCreateInfo queueCreateInfo{};
  queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
  queueCreateInfo.queueCount = 1;
  queueCreateInfo.pQueuePriorities = &queuePriority;

  VkDeviceQueueCreateInfo queueCreateInfos[2];
  int count = 0;

  {
    queueCreateInfo.queueFamilyIndex = indices.graphicsFamily;
    queueCreateInfos[count++] = queueCreateInfo;
  }

  if(indices.graphicsFamily != indices.presentFamily)
  {
    queueCreateInfo.queueFamilyIndex = indices.presentFamily;
    queueCreateInfos[count++] = queueCreateInfo;
  }

  VkDeviceCreateInfo createInfo{};
  createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
  createInfo.queueCreateInfoCount = count;
  createInfo.pQueueCreateInfos = queueCreateInfos;
  createInfo.enabledExtensionCount = lengthof(RequiredDeviceExtensions);
  createInfo.ppEnabledExtensionNames = RequiredDeviceExtensions;

  VkDevice device;

  if(vkCreateDevice(physicalDevice, &createInfo, nullptr, &device) != VK_SUCCESS)
    throw std::runtime_error("failed to create logical device");

  vkGetDeviceQueue(device, indices.graphicsFamily, 0, graphicsQueue);
  vkGetDeviceQueue(device, indices.presentFamily, 0, presentQueue);

  return device;
}

static VkRenderPass createRenderPass(VkFormat swapchainImageFormat, VkDevice device)
{
  VkAttachmentDescription colorAttachment{};
  colorAttachment.format = swapchainImageFormat;
  colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
  colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
  colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
  colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
  colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
  colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

  VkAttachmentReference colorAttachmentRef{};
  colorAttachmentRef.attachment = 0;
  colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

  VkSubpassDescription subpass{};
  subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
  subpass.colorAttachmentCount = 1;
  subpass.pColorAttachments = &colorAttachmentRef;

  VkSubpassDependency dependency{};
  dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
  dependency.dstSubpass = 0;
  dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
  dependency.srcAccessMask = 0;
  dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
  dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

  VkRenderPassCreateInfo renderPassInfo{};
  renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
  renderPassInfo.attachmentCount = 1;
  renderPassInfo.pAttachments = &colorAttachment;
  renderPassInfo.subpassCount = 1;
  renderPassInfo.pSubpasses = &subpass;
  renderPassInfo.dependencyCount = 1;
  renderPassInfo.pDependencies = &dependency;

  VkRenderPass renderPass;
  if(vkCreateRenderPass(device, &renderPassInfo, nullptr, &renderPass) != VK_SUCCESS)
    throw std::runtime_error("failed to create render pass");

  return renderPass;
}

static VkFramebuffer createFramebuffer(VkDevice device, VkImageView imgview, VkExtent2D extent, VkRenderPass renderPass)
{
  VkImageView attachments[] = {imgview};

  VkFramebufferCreateInfo framebufferInfo{};
  framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
  framebufferInfo.renderPass = renderPass;
  framebufferInfo.attachmentCount = 1;
  framebufferInfo.pAttachments = attachments;
  framebufferInfo.width = extent.width;
  framebufferInfo.height = extent.height;
  framebufferInfo.layers = 1;

  VkFramebuffer framebuffer;

  if(vkCreateFramebuffer(device, &framebufferInfo, nullptr, &framebuffer) != VK_SUCCESS)
    throw std::runtime_error("failed to create framebuffer");

  return framebuffer;
}

static VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats)
{
  for(const auto& availableFormat : availableFormats)
  {
    if(availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
    {
      return availableFormat;
    }
  }

  return availableFormats[0];
}

static VkExtent2D chooseSwapExtent(SDL_Window* window, const VkSurfaceCapabilitiesKHR& capabilities)
{
  if(capabilities.currentExtent.width != UINT32_MAX)
    return capabilities.currentExtent;

  int width, height;
  SDL_Vulkan_GetDrawableSize(window, &width, &height);

  VkExtent2D actualExtent = {static_cast<uint32_t>(width), static_cast<uint32_t>(height)};

  actualExtent.height = clamp(actualExtent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);
  actualExtent.width = clamp(actualExtent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);

  return actualExtent;
}

class ApplicationHost
{
public:
  const AppCreationFunc hostedAppCreationFunc{};
  ApplicationHost(const char* appName, AppCreationFunc creationFunc)
      : hostedAppCreationFunc(creationFunc)
  {
    m_camera.mat = lookAt({3, 3, 3}, {}, {0, 0, 1});

    initWindow(appName);
    initVulkan();
  }

  ~ApplicationHost()
  {
    cleanupSwapChain();

    vkDestroyCommandPool(device, commandPool, nullptr);

    vkDestroyDevice(device, nullptr);

    vkDestroySurfaceKHR(instance, surface, nullptr);
    vkDestroyInstance(instance, nullptr);

    SDL_DestroyWindow(window);

    SDL_QuitSubSystem(SDL_INIT_VIDEO);
  }

  void run()
  {
    int frames = 0;
    double t0 = SDL_GetTicks() / 1000.0;

    double lastDate = t0;
    bool keepGoing = true;
    while(keepGoing)
    {
      double currDate = SDL_GetTicks() / 1000.0;
      double dt = currDate - lastDate;
      SDL_Event event;

      float dx = 0;
      float dy = 0;

      while(SDL_PollEvent(&event))
      {
        if(event.type == SDL_QUIT)
        {
          keepGoing = false;
          break;
        }
        else if(event.type == SDL_KEYDOWN)
        {
          if(event.key.keysym.sym == SDLK_ESCAPE)
          {
            keepGoing = false;
            break;
          }
        }
        else if(event.type == SDL_MOUSEMOTION)
        {
          Uint32 state = SDL_GetMouseState(nullptr, nullptr);
          if(state & SDL_BUTTON_LMASK)
          {
            dx += event.motion.xrel;
            dy += event.motion.yrel;
          }
        }
      }

      if(dx != 0 || dy != 0)
      {
        const auto speed = 0.01;
        m_camera.mat = rotateY(dx * speed) * rotateX(dy * speed) * m_camera.mat;
        normalizeMatrix(m_camera.mat);
      }

      {
        const auto state = SDL_GetKeyboardState(nullptr);

        const bool goLeft = state[SDL_SCANCODE_A];
        const bool goRight = state[SDL_SCANCODE_D];
        const bool goUp = state[SDL_SCANCODE_W];
        const bool goDown = state[SDL_SCANCODE_S];

        const auto speed = 10;

        Vec3f tx{};
        if(goLeft)
          tx += {+1, 0, 0};
        if(goRight)
          tx += {-1, 0, 0};
        if(goUp)
          tx += {0, 0, +1};
        if(goDown)
          tx += {0, 0, -1};

        m_camera.mat = translate(tx * speed * dt) * m_camera.mat;
      }

      drawFrame();
      lastDate = currDate;
      ++frames;
    }

    double t1 = SDL_GetTicks() / 1000.0;
    fprintf(stderr, "Avg FPS: %.2f FPS\n", frames / (t1 - t0));
  }

private:
  SDL_Window* window;

  VkInstance instance{};
  VkSurfaceKHR surface{};

  VkPhysicalDevice physicalDevice{};
  VkDevice device{};

  VkQueue graphicsQueue{};
  VkQueue presentQueue{};

  VkSwapchainKHR swapchain{};
  VkFormat swapchainImageFormat{};
  VkExtent2D swapchainExtent{};

  VkRenderPass renderPass{};
  VkCommandPool commandPool{};

  struct SwapChainImage
  {
    VkImage image; // not-owned
    VkImageView view;
    VkFramebuffer framebuffer;
    VkCommandBuffer commandBuffer;
    int syncIdx = -1; // index into 'frameSync[]'
  };

  std::vector<SwapChainImage> swapchainImages;

  struct Frame
  {
    VkSemaphore availableForWriting;
    VkSemaphore renderFinished_forGPU;
    VkFence renderFinished_forCPU; // signaled when the cmd queue for this frame is processed
  };

  std::vector<Frame> frameSync;
  size_t currFrameSync = 0;

  bool framebufferResized = false;

  void initWindow(const char* appName)
  {
    SDL_InitSubSystem(SDL_INIT_VIDEO);

    if(SDL_Vulkan_LoadLibrary(nullptr) != 0)
      throw std::runtime_error("Couldn't load Vulkan library");

    g_vkGetInstanceProcAddr = decltype(g_vkGetInstanceProcAddr)(SDL_Vulkan_GetVkGetInstanceProcAddr());
    if(!g_vkGetInstanceProcAddr)
      throw std::runtime_error("Couldn't resolve 'vkGetInstanceProcAddr'\n");

    std::string title = "Vulkanisch - " + std::string(appName);
    window = SDL_CreateWindow(title.c_str(), 0, 0, WIDTH, HEIGHT, SDL_WINDOW_VULKAN);
    if(!window)
      throw std::runtime_error("Couldn't create window");

    SDL_SetRelativeMouseMode(SDL_TRUE);
  }

  void initVulkan()
  {
    // bootstrap vulkan, stage 1/3: we don't know the instance nor the physical device
    loadVulkanUsingGlad(VK_NULL_HANDLE, VK_NULL_HANDLE);

    instance = createInstance();

    // bootstrap vulkan, stage 2/3: we know the instance, but still not the physical device
    loadVulkanUsingGlad(instance, VK_NULL_HANDLE);

    surface = createSurface(instance, window);
    physicalDevice = pickPhysicalDevice(instance, surface);

    // bootstrap vulkan, stage 3/3: we know the instance and the device
    loadVulkanUsingGlad(instance, physicalDevice);

    device = createLogicalDevice(physicalDevice, surface, &graphicsQueue, &presentQueue);
    createCommandPool();

    recreateSwapChain();

    {
      VkPhysicalDeviceProperties props{};
      vkGetPhysicalDeviceProperties(physicalDevice, &props);
      fprintf(stderr, "Using device: %s\n", props.deviceName);
    }
  }

  void recreateSwapChain()
  {
    cleanupSwapChain();
    createSwapChain(physicalDevice, device, surface);

    renderPass = createRenderPass(swapchainImageFormat, device);

    for(auto& swimg : swapchainImages)
    {
      swimg.view = createImageView(device, swimg.image, swapchainImageFormat);
      swimg.framebuffer = createFramebuffer(device, swimg.view, swapchainExtent, renderPass);
    }

    fprintf(stderr, "Swapchain created with %d images\n", (int)swapchainImages.size());

    createCommandBuffers();
    createSyncObjects();

    AppCreationContext ctx{};
    ctx.device = device;
    ctx.physicalDevice = physicalDevice;
    ctx.swapchainExtent = swapchainExtent;
    ctx.renderPass = renderPass;
    hostedApp.reset(hostedAppCreationFunc(ctx));
  }

  void cleanupSwapChain()
  {
    vkDeviceWaitIdle(device);

    hostedApp.reset();

    for(auto& image : swapchainImages)
    {
      vkDestroyFramebuffer(device, image.framebuffer, nullptr);
      vkDestroyImageView(device, image.view, nullptr);
      vkFreeCommandBuffers(device, commandPool, 1, &image.commandBuffer);
    }

    vkDestroyRenderPass(device, renderPass, nullptr);
    vkDestroySwapchainKHR(device, swapchain, nullptr);

    for(auto frame : frameSync)
    {
      vkDestroySemaphore(device, frame.availableForWriting, nullptr);
      vkDestroySemaphore(device, frame.renderFinished_forGPU, nullptr);
      vkDestroyFence(device, frame.renderFinished_forCPU, nullptr);
    }
  }

  void createSwapChain(VkPhysicalDevice physicalDevice, VkDevice device, VkSurfaceKHR surface)
  {
    const SwapChainSupportDetails details = querySwapChainSupport(physicalDevice, surface);

    const VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(details.formats);
    const VkExtent2D extent = chooseSwapExtent(window, details.capabilities);

    const uint32_t maxImageCount = details.capabilities.maxImageCount > 0 ? details.capabilities.maxImageCount : UINT_MAX;
    uint32_t imageCount = std::min(details.capabilities.minImageCount + 1, maxImageCount);

    VkSwapchainCreateInfoKHR createInfo{};

    createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    createInfo.surface = surface;
    createInfo.minImageCount = imageCount;
    createInfo.imageFormat = surfaceFormat.format;
    createInfo.imageColorSpace = surfaceFormat.colorSpace;
    createInfo.imageExtent = extent;
    createInfo.imageArrayLayers = 1;
    createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    QueueFamilyIndices indices = findQueueFamilies(physicalDevice, surface);
    uint32_t queueFamilyIndices[] = {indices.graphicsFamily, indices.presentFamily};

    if(indices.graphicsFamily != indices.presentFamily)
    {
      createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
      createInfo.queueFamilyIndexCount = 2;
      createInfo.pQueueFamilyIndices = queueFamilyIndices;
    }
    else
    {
      createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    }

    createInfo.preTransform = details.capabilities.currentTransform;
    createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    createInfo.presentMode = VK_PRESENT_MODE_FIFO_KHR;
    createInfo.clipped = VK_TRUE;

    if(vkCreateSwapchainKHR(device, &createInfo, nullptr, &swapchain) != VK_SUCCESS)
      throw std::runtime_error("failed to create swap chain");

    vkGetSwapchainImagesKHR(device, swapchain, &imageCount, nullptr);
    std::vector<VkImage> images(imageCount);
    vkGetSwapchainImagesKHR(device, swapchain, &imageCount, images.data());

    swapchainImages.resize(images.size());

    for(int i = 0; i < (int)images.size(); ++i)
    {
      swapchainImages[i].image = images[i];
    }

    swapchainImageFormat = surfaceFormat.format;
    swapchainExtent = extent;

    fprintf(stderr, "Created swap chain: %dx%d (%d images)\n", extent.width, extent.height, imageCount);
  }

  void createCommandPool()
  {
    QueueFamilyIndices queueFamilyIndices = findQueueFamilies(physicalDevice, surface);

    VkCommandPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily;
    poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

    if(vkCreateCommandPool(device, &poolInfo, nullptr, &commandPool) != VK_SUCCESS)
      throw std::runtime_error("failed to create command pool");
  }

  void createCommandBuffers()
  {
    for(auto& swimg : swapchainImages)
    {
      VkCommandBufferAllocateInfo allocInfo{};
      allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
      allocInfo.commandPool = commandPool;
      allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
      allocInfo.commandBufferCount = 1;

      if(vkAllocateCommandBuffers(device, &allocInfo, &swimg.commandBuffer) != VK_SUCCESS)
        throw std::runtime_error("failed to allocate command buffers");
    }
  }

  void createSyncObjects()
  {
    frameSync.resize(2);

    for(auto& frame : frameSync)
    {
      VkSemaphoreCreateInfo semaphoreInfo{};
      semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

      if(vkCreateSemaphore(device, &semaphoreInfo, nullptr, &frame.availableForWriting) != VK_SUCCESS)
        throw std::runtime_error("failed to create semaphore for a frame");

      if(vkCreateSemaphore(device, &semaphoreInfo, nullptr, &frame.renderFinished_forGPU) != VK_SUCCESS)
        throw std::runtime_error("failed to create semaphore for a frame");

      VkFenceCreateInfo fenceInfo{};
      fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
      fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

      if(vkCreateFence(device, &fenceInfo, nullptr, &frame.renderFinished_forCPU) != VK_SUCCESS)
        throw std::runtime_error("failed to create synchronization objects for a frame");
    }
  }

  void drawFrame()
  {
    vkWaitForFences(device, 1, &frameSync[currFrameSync].renderFinished_forCPU, VK_TRUE, UINT64_MAX);

    uint32_t imageIndex;
    VkResult result = vkAcquireNextImageKHR(device, swapchain, UINT64_MAX, frameSync[currFrameSync].availableForWriting, VK_NULL_HANDLE, &imageIndex);

    if(result == VK_ERROR_OUT_OF_DATE_KHR)
    {
      recreateSwapChain();
      return;
    }

    if(result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR)
      throw std::runtime_error("failed to acquire swap chain image");

    auto& nextImage = swapchainImages[imageIndex];

    if(nextImage.syncIdx != -1)
    {
      vkWaitForFences(device, 1, &frameSync[nextImage.syncIdx].renderFinished_forCPU, VK_TRUE, UINT64_MAX);
    }

    nextImage.syncIdx = currFrameSync;

    // draw
    recordCommandBuffer(nextImage.commandBuffer, nextImage.framebuffer);

    // submit
    {
      const VkPipelineStageFlags waitStages = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

      VkSubmitInfo submitInfo{};

      submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
      submitInfo.waitSemaphoreCount = 1;
      submitInfo.pWaitSemaphores = &frameSync[currFrameSync].availableForWriting;
      submitInfo.pWaitDstStageMask = &waitStages;
      submitInfo.commandBufferCount = 1;
      submitInfo.pCommandBuffers = &nextImage.commandBuffer;
      submitInfo.signalSemaphoreCount = 1;
      submitInfo.pSignalSemaphores = &frameSync[currFrameSync].renderFinished_forGPU;

      vkResetFences(device, 1, &frameSync[currFrameSync].renderFinished_forCPU);

      if(vkQueueSubmit(graphicsQueue, 1, &submitInfo, frameSync[currFrameSync].renderFinished_forCPU) != VK_SUCCESS)
        throw std::runtime_error("failed to submit draw command buffer");
    }

    // present
    {
      VkPresentInfoKHR presentInfo{};

      presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
      presentInfo.waitSemaphoreCount = 1;
      presentInfo.pWaitSemaphores = &frameSync[currFrameSync].renderFinished_forGPU;
      presentInfo.swapchainCount = 1;
      presentInfo.pSwapchains = &swapchain;
      presentInfo.pImageIndices = &imageIndex;

      result = vkQueuePresentKHR(presentQueue, &presentInfo);

      if(result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || framebufferResized)
      {
        framebufferResized = false;
        recreateSwapChain();
      }
      else if(result != VK_SUCCESS)
        throw std::runtime_error("failed to present swap chain image");
    }

    currFrameSync = (currFrameSync + 1) % frameSync.size();
  }

  void recordCommandBuffer(VkCommandBuffer commandBuffer, VkFramebuffer framebuffer)
  {
    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

    if(vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS)
      throw std::runtime_error("failed to begin recording command buffer");

    hostedApp->setCamera(m_camera);
    hostedApp->drawFrame(SDL_GetTicks() / 1000.0, framebuffer, commandBuffer);

    if(vkEndCommandBuffer(commandBuffer) != VK_SUCCESS)
      throw std::runtime_error("failed to record command buffer");
  }

  std::unique_ptr<IApp> hostedApp;
  Camera m_camera;
};

int main(int argc, char* argv[])
{
  try
  {
    const char* appName = "HelloCube";

    if(argc >= 2)
      appName = argv[1];

    auto i = Registry().find(appName);
    if(i == Registry().end())
    {
      fprintf(stderr, "App not found: '%s'\n", appName);
      return 1;
    }

    ApplicationHost app(appName, i->second);
    app.run();
    return 0;
  }
  catch(const std::exception& e)
  {
    fprintf(stderr, "Fatal: %s\n", e.what());
    return 1;
  }
}
