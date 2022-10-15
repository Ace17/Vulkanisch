#include "common/app.h"
#include "common/matrix4.h"
#include "common/util.h"
#include "common/vkutil.h"

#include <cstring> // memcpy
#include <stdexcept>
#include <vector>

namespace
{
const VkFormat DepthFormat = VK_FORMAT_D16_UNORM;
const int ShadowMapSize = 4096;

///////////////////////////////////////////////////////////////////////////////
// Vertex

struct Vertex
{
  float x, y, z;
  float u, v;
};

constexpr VkVertexInputBindingDescription bindingDesc[] = {
      // stride
      {
            .binding = 0,
            .stride = sizeof(Vertex),
            .inputRate = VK_VERTEX_INPUT_RATE_VERTEX,
      }};

constexpr VkVertexInputAttributeDescription attributeDesc[] = {
      // position
      {
            .location = 0,
            .binding = 0,
            .format = VK_FORMAT_R32G32B32_SFLOAT,
            .offset = offsetof(Vertex, x),
      },
      // color
      {
            .location = 1,
            .binding = 0,
            .format = VK_FORMAT_R32G32_SFLOAT,
            .offset = offsetof(Vertex, u),
      }};

const Vertex vertices[] = {

      // Floor
      {/*pos*/ +8.0f, +8.0f, -2.0f, /*uv*/ 1, 1},
      {/*pos*/ -8.0f, +8.0f, -2.0f, /*uv*/ 0, 1},
      {/*pos*/ -8.0f, -8.0f, -2.0f, /*uv*/ 0, 0},
      {/*pos*/ +8.0f, +8.0f, -2.0f, /*uv*/ 1, 1},
      {/*pos*/ -8.0f, -8.0f, -2.0f, /*uv*/ 0, 0},
      {/*pos*/ +8.0f, -8.0f, -2.0f, /*uv*/ 1, 0},

      // Cube
      {/*pos*/ -1.0f, -1.0f, -1.0f, /*uv*/ 0, 0},
      {/*pos*/ -1.0f, -1.0f, +1.0f, /*uv*/ 0, 1},
      {/*pos*/ -1.0f, +1.0f, +1.0f, /*uv*/ 1, 1},

      {/*pos*/ +1.0f, +1.0f, -1.0f, /*uv*/ 1, 1},
      {/*pos*/ -1.0f, -1.0f, -1.0f, /*uv*/ 0, 0},
      {/*pos*/ -1.0f, +1.0f, -1.0f, /*uv*/ 0, 1},

      {/*pos*/ +1.0f, -1.0f, +1.0f, /*uv*/ 1, 1},
      {/*pos*/ -1.0f, -1.0f, -1.0f, /*uv*/ 0, 0},
      {/*pos*/ +1.0f, -1.0f, -1.0f, /*uv*/ 1, 0},

      {/*pos*/ +1.0f, +1.0f, -1.0f, /*uv*/ 1, 1},
      {/*pos*/ +1.0f, -1.0f, -1.0f, /*uv*/ 1, 0},
      {/*pos*/ -1.0f, -1.0f, -1.0f, /*uv*/ 0, 0},

      {/*pos*/ -1.0f, -1.0f, -1.0f, /*uv*/ 0, 0},
      {/*pos*/ -1.0f, +1.0f, +1.0f, /*uv*/ 1, 1},
      {/*pos*/ -1.0f, +1.0f, -1.0f, /*uv*/ 1, 0},

      {/*pos*/ +1.0f, -1.0f, +1.0f, /*uv*/ 1, 1},
      {/*pos*/ -1.0f, -1.0f, +1.0f, /*uv*/ 0, 1},
      {/*pos*/ -1.0f, -1.0f, -1.0f, /*uv*/ 0, 0},

      {/*pos*/ -1.0f, +1.0f, +1.0f, /*uv*/ 0, 1},
      {/*pos*/ -1.0f, -1.0f, +1.0f, /*uv*/ 0, 0},
      {/*pos*/ +1.0f, -1.0f, +1.0f, /*uv*/ 1, 0},

      {/*pos*/ +1.0f, +1.0f, +1.0f, /*uv*/ 1, 1},
      {/*pos*/ +1.0f, -1.0f, -1.0f, /*uv*/ 0, 0},
      {/*pos*/ +1.0f, +1.0f, -1.0f, /*uv*/ 1, 0},

      {/*pos*/ +1.0f, -1.0f, -1.0f, /*uv*/ 0, 0},
      {/*pos*/ +1.0f, +1.0f, +1.0f, /*uv*/ 1, 1},
      {/*pos*/ +1.0f, -1.0f, +1.0f, /*uv*/ 0, 1},

      {/*pos*/ +1.0f, +1.0f, +1.0f, /*uv*/ 1, 1},
      {/*pos*/ +1.0f, +1.0f, -1.0f, /*uv*/ 1, 0},
      {/*pos*/ -1.0f, +1.0f, -1.0f, /*uv*/ 0, 0},

      {/*pos*/ +1.0f, +1.0f, +1.0f, /*uv*/ 1, 1},
      {/*pos*/ -1.0f, +1.0f, -1.0f, /*uv*/ 0, 0},
      {/*pos*/ -1.0f, +1.0f, +1.0f, /*uv*/ 0, 1},

      {/*pos*/ +1.0f, +1.0f, +1.0f, /*uv*/ 1, 1},
      {/*pos*/ -1.0f, +1.0f, +1.0f, /*uv*/ 0, 1},
      {/*pos*/ +1.0f, -1.0f, +1.0f, /*uv*/ 1, 0},
};

VkDeviceMemory createBufferMemory(VkPhysicalDevice physicalDevice, VkDevice device, VkBuffer buffer)
{
  VkDeviceMemory memory{};
  VkMemoryRequirements memRequirements;
  vkGetBufferMemoryRequirements(device, buffer, &memRequirements);

  VkMemoryAllocateInfo allocInfo{};
  allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
  allocInfo.allocationSize = memRequirements.size;
  allocInfo.memoryTypeIndex =
        findMemoryType(physicalDevice, memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

  if(vkAllocateMemory(device, &allocInfo, nullptr, &memory) != VK_SUCCESS)
    throw std::runtime_error("failed to allocate vertex buffer memory");

  vkBindBufferMemory(device, buffer, memory, 0);

  return memory;
}

VkDescriptorSetLayout createDescriptorSetLayout(VkDevice device)
{
  VkDescriptorSetLayoutBinding setLayoutBindings[2]{};

  // Binding 0: sampler
  setLayoutBindings[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  setLayoutBindings[0].binding = 0;
  setLayoutBindings[0].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
  setLayoutBindings[0].descriptorCount = 1;

  // Binding 1: uniform buffer
  setLayoutBindings[1].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
  setLayoutBindings[1].binding = 1;
  setLayoutBindings[1].stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
  setLayoutBindings[1].descriptorCount = 1;

  // Create the descriptor set layout
  VkDescriptorSetLayoutCreateInfo info{};
  info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
  info.bindingCount = lengthof(setLayoutBindings);
  info.pBindings = setLayoutBindings;

  VkDescriptorSetLayout descriptorSetLayout;
  vkCreateDescriptorSetLayout(device, &info, nullptr, &descriptorSetLayout);

  return descriptorSetLayout;
}

VkDescriptorPool createDescriptorPool(VkDevice device)
{
  VkDescriptorPoolSize sizes[2];

  // Uniform buffers
  sizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
  sizes[0].descriptorCount = 2;

  // Samplers
  sizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  sizes[1].descriptorCount = 2;

  // Create the global descriptor pool
  VkDescriptorPoolCreateInfo info{};
  info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
  info.poolSizeCount = lengthof(sizes);
  info.pPoolSizes = sizes;
  info.maxSets = 2; // number of desc sets that can be allocated from this pool

  VkDescriptorPool descriptorPool;
  vkCreateDescriptorPool(device, &info, nullptr, &descriptorPool);
  return descriptorPool;
}

VkPipelineLayout createPipelineLayout(VkDevice device, VkDescriptorSetLayout descriptorSetLayout)
{
  VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
  pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
  pipelineLayoutInfo.setLayoutCount = 1;
  pipelineLayoutInfo.pSetLayouts = &descriptorSetLayout;

  VkPipelineLayout pipelineLayout;

  if(vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &pipelineLayout) != VK_SUCCESS)
    throw std::runtime_error("failed to create pipeline layout");

  return pipelineLayout;
}

VkPipeline createShadowMapPipeline(VkDevice device, VkPipelineLayout pipelineLayout, VkRenderPass renderPass)
{
  auto vertShaderCode = loadFile("bin/src/shadowmap/shader.vert.spv");

  VkShaderModule vertShaderModule = createShaderModule(device, vertShaderCode);

  VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
  vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
  vertShaderStageInfo.module = vertShaderModule;
  vertShaderStageInfo.pName = "main";

  VkPipelineShaderStageCreateInfo shaderStages[] = {vertShaderStageInfo};

  VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
  vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
  vertexInputInfo.vertexBindingDescriptionCount = lengthof(bindingDesc);
  vertexInputInfo.pVertexBindingDescriptions = bindingDesc;
  vertexInputInfo.vertexAttributeDescriptionCount = lengthof(attributeDesc);
  vertexInputInfo.pVertexAttributeDescriptions = attributeDesc;

  VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
  inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
  inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
  inputAssembly.primitiveRestartEnable = VK_FALSE;

  VkViewport viewport{};
  viewport.x = 0.0f;
  viewport.y = (float)ShadowMapSize;
  viewport.width = (float)ShadowMapSize;
  viewport.height = -(float)ShadowMapSize;
  viewport.minDepth = 0.0f;
  viewport.maxDepth = 1.0f;

  VkRect2D scissor{};
  scissor.offset = {0, 0};
  scissor.extent = {ShadowMapSize, ShadowMapSize};

  VkPipelineViewportStateCreateInfo viewportState{};
  viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
  viewportState.viewportCount = 1;
  viewportState.pViewports = &viewport;
  viewportState.scissorCount = 1;
  viewportState.pScissors = &scissor;

  VkPipelineRasterizationStateCreateInfo rasterizer{};
  rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
  rasterizer.depthClampEnable = VK_FALSE;
  rasterizer.rasterizerDiscardEnable = VK_FALSE;
  rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
  rasterizer.lineWidth = 1.0f;
  rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
  rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
  rasterizer.depthBiasEnable = VK_FALSE;

  VkPipelineMultisampleStateCreateInfo multisampling{};
  multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
  multisampling.sampleShadingEnable = VK_FALSE;
  multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

  VkPipelineDepthStencilStateCreateInfo depthStencilStateInfo{};
  depthStencilStateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
  depthStencilStateInfo.depthTestEnable = VK_TRUE;
  depthStencilStateInfo.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
  depthStencilStateInfo.depthWriteEnable = VK_TRUE;

  VkGraphicsPipelineCreateInfo pipelineInfo{};
  pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
  pipelineInfo.stageCount = lengthof(shaderStages);
  pipelineInfo.pStages = shaderStages;
  pipelineInfo.pVertexInputState = &vertexInputInfo;
  pipelineInfo.pInputAssemblyState = &inputAssembly;
  pipelineInfo.pViewportState = &viewportState;
  pipelineInfo.pRasterizationState = &rasterizer;
  pipelineInfo.pMultisampleState = &multisampling;
  pipelineInfo.pDepthStencilState = &depthStencilStateInfo;
  pipelineInfo.layout = pipelineLayout;
  pipelineInfo.renderPass = renderPass;
  pipelineInfo.subpass = 0;
  pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;

  VkPipeline pipeline{};

  if(vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &pipeline) != VK_SUCCESS)
    throw std::runtime_error("failed to create graphics pipeline");

  vkDestroyShaderModule(device, vertShaderModule, nullptr);

  return pipeline;
}

VkPipeline createGraphicsPipeline(VkDevice device, VkPipelineLayout pipelineLayout, VkExtent2D swapchainExtent, VkRenderPass renderPass)
{
  auto vertShaderCode = loadFile("bin/src/shadowmap/shader.vert.spv");
  auto fragShaderCode = loadFile("bin/src/shadowmap/shader.frag.spv");

  VkShaderModule vertShaderModule = createShaderModule(device, vertShaderCode);
  VkShaderModule fragShaderModule = createShaderModule(device, fragShaderCode);

  VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
  vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
  vertShaderStageInfo.module = vertShaderModule;
  vertShaderStageInfo.pName = "main";

  VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
  fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
  fragShaderStageInfo.module = fragShaderModule;
  fragShaderStageInfo.pName = "main";

  VkPipelineShaderStageCreateInfo shaderStages[] = {vertShaderStageInfo, fragShaderStageInfo};

  VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
  vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
  vertexInputInfo.vertexBindingDescriptionCount = lengthof(bindingDesc);
  vertexInputInfo.pVertexBindingDescriptions = bindingDesc;
  vertexInputInfo.vertexAttributeDescriptionCount = lengthof(attributeDesc);
  vertexInputInfo.pVertexAttributeDescriptions = attributeDesc;

  VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
  inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
  inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
  inputAssembly.primitiveRestartEnable = VK_FALSE;

  VkViewport viewport{};
  viewport.x = 0.0f;
  viewport.y = (float)swapchainExtent.height;
  viewport.width = (float)swapchainExtent.width;
  viewport.height = -((float)swapchainExtent.height);
  viewport.minDepth = 0.0f;
  viewport.maxDepth = 1.0f;

  VkRect2D scissor{};
  scissor.offset = {0, 0};
  scissor.extent = swapchainExtent;

  VkPipelineViewportStateCreateInfo viewportState{};
  viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
  viewportState.viewportCount = 1;
  viewportState.pViewports = &viewport;
  viewportState.scissorCount = 1;
  viewportState.pScissors = &scissor;

  VkPipelineRasterizationStateCreateInfo rasterizer{};
  rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
  rasterizer.depthClampEnable = VK_FALSE;
  rasterizer.rasterizerDiscardEnable = VK_FALSE;
  rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
  rasterizer.lineWidth = 1.0f;
  rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
  rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
  rasterizer.depthBiasEnable = VK_FALSE;

  VkPipelineMultisampleStateCreateInfo multisampling{};
  multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
  multisampling.sampleShadingEnable = VK_FALSE;
  multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

  VkPipelineColorBlendAttachmentState colorBlendAttachment{};
  colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
  colorBlendAttachment.blendEnable = VK_FALSE;

  VkPipelineColorBlendStateCreateInfo colorBlending{};
  colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
  colorBlending.logicOpEnable = VK_FALSE;
  colorBlending.logicOp = VK_LOGIC_OP_COPY;
  colorBlending.attachmentCount = 1;
  colorBlending.pAttachments = &colorBlendAttachment;
  colorBlending.blendConstants[0] = 0.0f;
  colorBlending.blendConstants[1] = 0.0f;
  colorBlending.blendConstants[2] = 0.0f;
  colorBlending.blendConstants[3] = 0.0f;

  VkGraphicsPipelineCreateInfo pipelineInfo{};
  pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
  pipelineInfo.stageCount = lengthof(shaderStages);
  pipelineInfo.pStages = shaderStages;
  pipelineInfo.pVertexInputState = &vertexInputInfo;
  pipelineInfo.pInputAssemblyState = &inputAssembly;
  pipelineInfo.pViewportState = &viewportState;
  pipelineInfo.pRasterizationState = &rasterizer;
  pipelineInfo.pMultisampleState = &multisampling;
  pipelineInfo.pColorBlendState = &colorBlending;
  pipelineInfo.layout = pipelineLayout;
  pipelineInfo.renderPass = renderPass;
  pipelineInfo.subpass = 0;
  pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;

  VkPipeline pipeline{};

  if(vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &pipeline) != VK_SUCCESS)
    throw std::runtime_error("failed to create graphics pipeline");

  vkDestroyShaderModule(device, fragShaderModule, nullptr);
  vkDestroyShaderModule(device, vertShaderModule, nullptr);

  return pipeline;
}

VkBuffer createVertexBuffer(VkDevice device, size_t size)
{
  VkBuffer vertexBuffer;
  VkBufferCreateInfo bufferInfo{};
  bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
  bufferInfo.size = size;
  bufferInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
  bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

  if(vkCreateBuffer(device, &bufferInfo, nullptr, &vertexBuffer) != VK_SUCCESS)
    throw std::runtime_error("failed to create vertex buffer");

  return vertexBuffer;
}

VkDescriptorSet createDescriptorSet(VkDevice device, VkDescriptorPool pool, VkDescriptorSetLayout layout)
{
  VkDescriptorSetAllocateInfo allocateInfo{};
  allocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
  allocateInfo.descriptorPool = pool;
  allocateInfo.descriptorSetCount = 1;
  allocateInfo.pSetLayouts = &layout;
  VkDescriptorSet descriptorSet;
  vkAllocateDescriptorSets(device, &allocateInfo, &descriptorSet);
  return descriptorSet;
}

struct VulkanTexture
{
  VkImage image;
  VkImageView view;
  VkDeviceMemory memory;
  VkSampler sampler;
  VkFramebuffer framebuffer;
};

VulkanTexture createFramebufferForShadowMap(VkDevice device, VkPhysicalDevice physicalDevice, int width, int height, VkRenderPass renderPass)
{
  VulkanTexture result{};

  // Create image for the depth buffer
  {
    VkImageCreateInfo info{};
    info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    info.imageType = VK_IMAGE_TYPE_2D;
    info.extent = {(uint32_t)width, (uint32_t)height, 1};
    info.format = DepthFormat;
    info.mipLevels = 1;
    info.arrayLayers = 1;
    info.samples = VK_SAMPLE_COUNT_1_BIT;
    info.tiling = VK_IMAGE_TILING_OPTIMAL;
    info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    info.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    vkCreateImage(device, &info, nullptr, &result.image);
  }

  // Allocate memory for the texture image
  {
    {
      VkMemoryRequirements memReqs;
      vkGetImageMemoryRequirements(device, result.image, &memReqs);

      VkMemoryAllocateInfo info{};
      info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
      info.allocationSize = memReqs.size;
      info.memoryTypeIndex = findMemoryType(physicalDevice, memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
      vkAllocateMemory(device, &info, nullptr, &result.memory);
    }

    vkBindImageMemory(device, result.image, result.memory, 0);
  }

  // Create image view
  {
    VkImageViewCreateInfo info{};
    info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    info.viewType = VK_IMAGE_VIEW_TYPE_2D;
    info.format = DepthFormat;
    info.subresourceRange = {VK_IMAGE_ASPECT_DEPTH_BIT, 0, 1, 0, 1};
    info.subresourceRange.levelCount = 1;
    info.image = result.image;
    vkCreateImageView(device, &info, nullptr, &result.view);
  }

  // Create sampler
  {
    VkSamplerCreateInfo info{};
    info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    info.magFilter = VK_FILTER_NEAREST;
    info.minFilter = VK_FILTER_NEAREST;
    info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
    info.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    info.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    info.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    info.mipLodBias = 0.0f;
    info.maxAnisotropy = 1.0f;
    info.minLod = 0.0f;
    info.maxLod = 1.0f;
    info.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
    vkCreateSampler(device, &info, nullptr, &result.sampler);
  }

  // Create framebuffer
  {
    VkFramebufferCreateInfo info{};
    info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    info.renderPass = renderPass;
    info.attachmentCount = 1;
    info.pAttachments = &result.view;
    info.width = width;
    info.height = height;
    info.layers = 1;

    vkCreateFramebuffer(device, &info, nullptr, &result.framebuffer);
  }

  return result;
}

void destroyTexture(VkDevice device, const VulkanTexture& texture)
{
  vkDestroyFramebuffer(device, texture.framebuffer, nullptr);
  vkDestroyImageView(device, texture.view, nullptr);
  vkDestroySampler(device, texture.sampler, nullptr);
  vkDestroyImage(device, texture.image, nullptr);
  vkFreeMemory(device, texture.memory, nullptr);
}

struct MyUniformBlock
{
  Matrix4f model;
  Matrix4f view;
  Matrix4f proj;
  Matrix4f LightMVP;
};

VkRenderPass createShadowMapRenderPass(VkDevice device)
{
  VkAttachmentDescription attachmentDescription{};
  attachmentDescription.format = DepthFormat;
  attachmentDescription.samples = VK_SAMPLE_COUNT_1_BIT;
  attachmentDescription.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
  attachmentDescription.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
  attachmentDescription.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
  attachmentDescription.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
  attachmentDescription.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  attachmentDescription.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL; // shader read

  // will be used as depth/stencil during render pass
  VkAttachmentReference depthReference = {};
  depthReference.attachment = 0;
  depthReference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

  VkSubpassDescription subpass = {};
  subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
  subpass.colorAttachmentCount = 0;
  subpass.pDepthStencilAttachment = &depthReference;

  // Use subpass dependencies for layout transitions
  VkSubpassDependency dependencies[2]{};

  dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
  dependencies[0].dstSubpass = 0;
  dependencies[0].srcStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
  dependencies[0].dstStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
  dependencies[0].srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
  dependencies[0].dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
  dependencies[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

  dependencies[1].srcSubpass = 0;
  dependencies[1].dstSubpass = VK_SUBPASS_EXTERNAL;
  dependencies[1].srcStageMask = VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
  dependencies[1].dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
  dependencies[1].srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
  dependencies[1].dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
  dependencies[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

  VkRenderPassCreateInfo info{};
  info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
  info.attachmentCount = 1;
  info.pAttachments = &attachmentDescription;
  info.subpassCount = 1;
  info.pSubpasses = &subpass;
  info.dependencyCount = lengthof(dependencies);
  info.pDependencies = dependencies;

  VkRenderPass renderPass{};
  vkCreateRenderPass(device, &info, nullptr, &renderPass);
  return renderPass;
}

class ShadowMap : public IApp
{
public:
  ShadowMap(const AppCreationContext& ctx_)
      : ctx(ctx_)
  {
    descriptorPool = createDescriptorPool(ctx.device);
    descriptorSetLayout = createDescriptorSetLayout(ctx.device);
    mainSceneDescriptorSet = createDescriptorSet(ctx.device, descriptorPool, descriptorSetLayout);
    shadowMapDescriptorSet = createDescriptorSet(ctx.device, descriptorPool, descriptorSetLayout);

    shadowMapRenderPass = createShadowMapRenderPass(ctx.device);

    pipelineLayout = createPipelineLayout(ctx.device, descriptorSetLayout);
    graphicsPipeline = createGraphicsPipeline(ctx.device, pipelineLayout, ctx.swapchainExtent, ctx.renderPass);
    shadowMapPipeline = createShadowMapPipeline(ctx.device, pipelineLayout, shadowMapRenderPass);

    // Create the vertex buffer and send it to the GPU
    vertexBuffer = createVertexBuffer(ctx.device, lengthof(vertices) * sizeof(vertices[0]));
    vertexBufferMemory = createBufferMemory(ctx.physicalDevice, ctx.device, vertexBuffer);
    writeToGpuMemory(ctx.device, vertexBufferMemory, vertices, lengthof(vertices) * sizeof(vertices[0]));

    {
      VkBufferCreateInfo info{};
      info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
      info.size = sizeof(MyUniformBlock);
      info.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
      info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

      if(vkCreateBuffer(ctx.device, &info, nullptr, &uniformBuffer) != VK_SUCCESS)
        throw std::runtime_error("failed to create uniform buffer");

      uniformBufferMemory = createBufferMemory(ctx.physicalDevice, ctx.device, uniformBuffer);
    }

    {
      VkBufferCreateInfo info{};
      info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
      info.size = sizeof(MyUniformBlock);
      info.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
      info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

      if(vkCreateBuffer(ctx.device, &info, nullptr, &shadowMapUniformBuffer) != VK_SUCCESS)
        throw std::runtime_error("failed to create uniform buffer");

      shadowMapUniformBufferMemory = createBufferMemory(ctx.physicalDevice, ctx.device, shadowMapUniformBuffer);
    }

    // Create the texture image
    shadowMap = createFramebufferForShadowMap(ctx.device, ctx.physicalDevice, ShadowMapSize, ShadowMapSize, shadowMapRenderPass);

    // fill descriptor set for main scene: 'mainSceneDescriptorSet'
    {
      VkWriteDescriptorSet writeInfo[2]{};

      VkDescriptorImageInfo infoBinding0{};
      infoBinding0.sampler = shadowMap.sampler;
      infoBinding0.imageView = shadowMap.view;
      infoBinding0.imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;

      // Binding 0: Sampler
      writeInfo[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
      writeInfo[0].dstSet = mainSceneDescriptorSet;
      writeInfo[0].dstBinding = 0;
      writeInfo[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
      writeInfo[0].pImageInfo = &infoBinding0;
      writeInfo[0].descriptorCount = 1;

      VkDescriptorBufferInfo infoBinding1{};
      infoBinding1.buffer = uniformBuffer;
      infoBinding1.range = sizeof(MyUniformBlock);

      // Binding 1: uniform buffer
      writeInfo[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
      writeInfo[1].dstSet = mainSceneDescriptorSet;
      writeInfo[1].dstBinding = 1;
      writeInfo[1].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
      writeInfo[1].pBufferInfo = &infoBinding1;
      writeInfo[1].descriptorCount = 1;

      vkUpdateDescriptorSets(ctx.device, lengthof(writeInfo), writeInfo, 0, nullptr);
    }

    // fill descriptor set for shadowmap scene: 'shadowMapDescriptorSet'
    {
      VkWriteDescriptorSet writeInfo[1]{};

      VkDescriptorBufferInfo infoBinding1{};
      infoBinding1.buffer = shadowMapUniformBuffer;
      infoBinding1.range = sizeof(MyUniformBlock);

      // Binding 1: uniform buffer
      writeInfo[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
      writeInfo[0].dstSet = shadowMapDescriptorSet;
      writeInfo[0].dstBinding = 1;
      writeInfo[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
      writeInfo[0].pBufferInfo = &infoBinding1;
      writeInfo[0].descriptorCount = 1;

      vkUpdateDescriptorSets(ctx.device, lengthof(writeInfo), writeInfo, 0, nullptr);
    }
  }

  ~ShadowMap()
  {
    destroyTexture(ctx.device, shadowMap);

    vkDestroyBuffer(ctx.device, shadowMapUniformBuffer, nullptr);
    vkDestroyBuffer(ctx.device, uniformBuffer, nullptr);
    vkFreeMemory(ctx.device, shadowMapUniformBufferMemory, nullptr);
    vkFreeMemory(ctx.device, uniformBufferMemory, nullptr);
    vkDestroyBuffer(ctx.device, vertexBuffer, nullptr);
    vkFreeMemory(ctx.device, vertexBufferMemory, nullptr);
    vkDestroyPipeline(ctx.device, shadowMapPipeline, nullptr);
    vkDestroyPipeline(ctx.device, graphicsPipeline, nullptr);
    vkDestroyPipelineLayout(ctx.device, pipelineLayout, nullptr);

    vkDestroyRenderPass(ctx.device, shadowMapRenderPass, nullptr);

    vkDestroyDescriptorSetLayout(ctx.device, descriptorSetLayout, nullptr);
    vkDestroyDescriptorPool(ctx.device, descriptorPool, nullptr);
  }

  Camera m_camera;

  void setCamera(const Camera& camera) override { m_camera = camera; }

  void drawFrame(double time, VkFramebuffer framebuffer, VkCommandBuffer commandBuffer) override
  {
    const float angle = time * 1.2;
    const Matrix4f model = rotateZ(angle * 0.3);

    const Matrix4f lightView = lookAt({6, 2, 7}, {}, {0, 0, 1});
    const Matrix4f lightProj = perspective(1.5, 1, 1, 100);
    const Matrix4f mvpLight = lightProj * lightView * model;

    {
      VkClearValue clearDepth{};
      clearDepth.depthStencil = {1.0, 0};

      VkRenderPassBeginInfo renderPassInfo{};
      renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
      renderPassInfo.renderPass = shadowMapRenderPass;
      renderPassInfo.framebuffer = shadowMap.framebuffer;
      renderPassInfo.renderArea.offset = {0, 0};
      renderPassInfo.renderArea.extent = {ShadowMapSize, ShadowMapSize};
      renderPassInfo.clearValueCount = 1;
      renderPassInfo.pClearValues = &clearDepth;

      vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

      vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, shadowMapPipeline);

      VkBuffer vertexBuffers[] = {vertexBuffer};
      VkDeviceSize offsets[] = {0};
      vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);

      vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &shadowMapDescriptorSet, 0, nullptr);

      MyUniformBlock constants{};
      constants.model = model;
      constants.view = lightView;
      constants.proj = lightProj;

      // convert row-major (app) to column-major (GLSL)
      constants.model = transpose(constants.model);
      constants.view = transpose(constants.view);
      constants.proj = transpose(constants.proj);

      writeToGpuMemory(ctx.device, shadowMapUniformBufferMemory, &constants, sizeof constants);

      vkCmdDraw(commandBuffer, lengthof(vertices), 1, 0, 0);

      vkCmdEndRenderPass(commandBuffer);
    }

    {
      VkClearValue clearColor{};
      clearColor.color.float32[0] = 0.1f;
      clearColor.color.float32[1] = 0.1f;
      clearColor.color.float32[2] = 0.1f;
      clearColor.color.float32[3] = 1.0f;

      VkRenderPassBeginInfo renderPassInfo{};
      renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
      renderPassInfo.renderPass = ctx.renderPass;
      renderPassInfo.framebuffer = framebuffer;
      renderPassInfo.renderArea.offset = {0, 0};
      renderPassInfo.renderArea.extent = ctx.swapchainExtent;
      renderPassInfo.clearValueCount = 1;
      renderPassInfo.pClearValues = &clearColor;

      vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

      vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline);

      VkBuffer vertexBuffers[] = {vertexBuffer};
      VkDeviceSize offsets[] = {0};
      vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);

      vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &mainSceneDescriptorSet, 0, nullptr);

      MyUniformBlock constants{};
      constants.model = model;
      constants.view = m_camera.mat;
      constants.proj = perspective(1.5, 4.0 / 3.0, 0.1, 100);
      constants.LightMVP = mvpLight;

      // convert row-major (app) to column-major (GLSL)
      constants.model = transpose(constants.model);
      constants.view = transpose(constants.view);
      constants.proj = transpose(constants.proj);
      constants.LightMVP = transpose(constants.LightMVP);

      writeToGpuMemory(ctx.device, uniformBufferMemory, &constants, sizeof constants);

      vkCmdDraw(commandBuffer, lengthof(vertices), 1, 0, 0);

      vkCmdEndRenderPass(commandBuffer);
    }
  }

private:
  VkPipelineLayout pipelineLayout{};
  VkPipeline graphicsPipeline{};
  VkPipeline shadowMapPipeline{};
  VkBuffer vertexBuffer{};
  VkDeviceMemory vertexBufferMemory{};
  VkDescriptorSetLayout descriptorSetLayout{};
  VkDescriptorPool descriptorPool{};
  VkDescriptorSet mainSceneDescriptorSet{};
  VkDescriptorSet shadowMapDescriptorSet{};
  VkBuffer uniformBuffer{};
  VkBuffer shadowMapUniformBuffer{};
  VkDeviceMemory uniformBufferMemory{};
  VkDeviceMemory shadowMapUniformBufferMemory{};
  VulkanTexture shadowMap{};
  VkRenderPass shadowMapRenderPass{};

  const AppCreationContext ctx;
};
} // namespace

REGISTER_APP(ShadowMap);
