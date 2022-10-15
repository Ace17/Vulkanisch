#include "common/app.h"
#include "common/matrix4.h"
#include "common/util.h"
#include "common/vkutil.h"

#include <cassert>
#include <stdexcept>
#include <vector>

namespace
{
///////////////////////////////////////////////////////////////////////////////
// Vertex

struct Vertex
{
  float x, y, z;
  float nx, ny, nz;
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
      // normal
      {
            .location = 1,
            .binding = 0,
            .format = VK_FORMAT_R32G32B32_SFLOAT,
            .offset = offsetof(Vertex, nx),
      },
};

const Vertex vertices[] = {
      {/*pos*/ -1.0f, -1.0f, -1.0f, /*N*/ -1, 0, 0},
      {/*pos*/ -1.0f, -1.0f, +1.0f, /*N*/ -1, 0, 0},
      {/*pos*/ -1.0f, +1.0f, +1.0f, /*N*/ -1, 0, 0},

      {/*pos*/ -1.0f, -1.0f, -1.0f, /*N*/ -1, 0, 0},
      {/*pos*/ -1.0f, +1.0f, +1.0f, /*N*/ -1, 0, 0},
      {/*pos*/ -1.0f, +1.0f, -1.0f, /*N*/ -1, 0, 0},

      {/*pos*/ +1.0f, +1.0f, -1.0f, /*N*/ 0, 0, -1},
      {/*pos*/ -1.0f, -1.0f, -1.0f, /*N*/ 0, 0, -1},
      {/*pos*/ -1.0f, +1.0f, -1.0f, /*N*/ 0, 0, -1},

      {/*pos*/ +1.0f, +1.0f, -1.0f, /*N*/ 0, 0, -1},
      {/*pos*/ +1.0f, -1.0f, -1.0f, /*N*/ 0, 0, -1},
      {/*pos*/ -1.0f, -1.0f, -1.0f, /*N*/ 0, 0, -1},

      {/*pos*/ +1.0f, -1.0f, +1.0f, /*N*/ 0, -1, 0},
      {/*pos*/ -1.0f, -1.0f, -1.0f, /*N*/ 0, -1, 0},
      {/*pos*/ +1.0f, -1.0f, -1.0f, /*N*/ 0, -1, 0},

      {/*pos*/ +1.0f, -1.0f, +1.0f, /*N*/ 0, -1, 0},
      {/*pos*/ -1.0f, -1.0f, +1.0f, /*N*/ 0, -1, 0},
      {/*pos*/ -1.0f, -1.0f, -1.0f, /*N*/ 0, -1, 0},

      {/*pos*/ +1.0f, +1.0f, +1.0f, /*N*/ 0, 0, 1},
      {/*pos*/ -1.0f, +1.0f, +1.0f, /*N*/ 0, 0, 1},
      {/*pos*/ +1.0f, -1.0f, +1.0f, /*N*/ 0, 0, 1},

      {/*pos*/ -1.0f, +1.0f, +1.0f, /*N*/ 0, 0, 1},
      {/*pos*/ -1.0f, -1.0f, +1.0f, /*N*/ 0, 0, 1},
      {/*pos*/ +1.0f, -1.0f, +1.0f, /*N*/ 0, 0, 1},

      {/*pos*/ +1.0f, +1.0f, +1.0f, /*N*/ 1, 0, 0},
      {/*pos*/ +1.0f, -1.0f, -1.0f, /*N*/ 1, 0, 0},
      {/*pos*/ +1.0f, +1.0f, -1.0f, /*N*/ 1, 0, 0},

      {/*pos*/ +1.0f, -1.0f, -1.0f, /*N*/ 1, 0, 0},
      {/*pos*/ +1.0f, +1.0f, +1.0f, /*N*/ 1, 0, 0},
      {/*pos*/ +1.0f, -1.0f, +1.0f, /*N*/ 1, 0, 0},

      {/*pos*/ +1.0f, +1.0f, +1.0f, /*N*/ 0, 1, 0},
      {/*pos*/ +1.0f, +1.0f, -1.0f, /*N*/ 0, 1, 0},
      {/*pos*/ -1.0f, +1.0f, -1.0f, /*N*/ 0, 1, 0},

      {/*pos*/ +1.0f, +1.0f, +1.0f, /*N*/ 0, 1, 0},
      {/*pos*/ -1.0f, +1.0f, -1.0f, /*N*/ 0, 1, 0},
      {/*pos*/ -1.0f, +1.0f, +1.0f, /*N*/ 0, 1, 0},
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

struct VulkanTexture
{
  VkImage image;
  VkImageView view;
  VkDeviceMemory memory;
  VkSampler sampler;
  VkFramebuffer framebuffer;
};

struct MyUniformBlock
{
  Matrix4f model;
  Matrix4f view;
  Matrix4f proj;
};

const auto HdrFormat = VK_FORMAT_R32G32B32A32_SFLOAT;

VkDescriptorSetLayout createDescriptorSetLayout(VkDevice device)
{
  VkDescriptorSetLayoutBinding setLayoutBindings[3]{};

  // Binding 0: uniform buffer
  setLayoutBindings[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
  setLayoutBindings[0].binding = 0;
  setLayoutBindings[0].stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
  setLayoutBindings[0].descriptorCount = 1;

  // Binding 1: sampler
  setLayoutBindings[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  setLayoutBindings[1].binding = 1;
  setLayoutBindings[1].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
  setLayoutBindings[1].descriptorCount = 1;

  // Binding 2: sampler
  setLayoutBindings[2].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  setLayoutBindings[2].binding = 2;
  setLayoutBindings[2].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
  setLayoutBindings[2].descriptorCount = 1;

  // Create the descriptor set layout
  VkDescriptorSetLayoutCreateInfo info{};
  info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
  info.bindingCount = lengthof(setLayoutBindings);
  info.pBindings = setLayoutBindings;

  VkDescriptorSetLayout descriptorSetLayout;
  vkCreateDescriptorSetLayout(device, &info, nullptr, &descriptorSetLayout);

  return descriptorSetLayout;
}

void setupDescriptorSet(VkDevice device, VkDescriptorSet ds, std::vector<VulkanTexture> inputTextures, VkBuffer uniformBuffer)
{
  std::vector<VkWriteDescriptorSet> writeDescriptorSets;

  // Binding 0: uniform buffer
  VkDescriptorBufferInfo uniformBufferInfo{};
  uniformBufferInfo.buffer = uniformBuffer;
  uniformBufferInfo.range = sizeof(MyUniformBlock);

  writeDescriptorSets.push_back({});
  auto& wds = writeDescriptorSets.back();
  wds.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  wds.dstSet = ds;
  wds.dstBinding = 0;
  wds.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
  wds.pBufferInfo = &uniformBufferInfo;
  wds.descriptorCount = 1;

  std::vector<VkDescriptorImageInfo> samplerInfo;
  samplerInfo.reserve(inputTextures.size());

  // Binding 1+i: Sampler
  for(auto& inputTexture : inputTextures)
  {
    auto i = int(&inputTexture - inputTextures.data());
    samplerInfo.push_back({});
    auto& si = samplerInfo.back();
    si.sampler = inputTexture.sampler;
    si.imageView = inputTexture.view;
    si.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    writeDescriptorSets.push_back({});
    auto& wds = writeDescriptorSets.back();
    wds.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    wds.dstSet = ds;
    wds.dstBinding = 1 + i;
    wds.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    wds.pImageInfo = &si;
    wds.descriptorCount = 1;
  }

  vkUpdateDescriptorSets(device, writeDescriptorSets.size(), writeDescriptorSets.data(), 0, nullptr);
}

VkDescriptorPool createDescriptorPool(VkDevice device)
{
  VkDescriptorPoolSize sizes[2];

  // Samplers
  sizes[0].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  sizes[0].descriptorCount = 16;

  // Uniform buffers
  sizes[1].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
  sizes[1].descriptorCount = 16;

  // Create the global descriptor pool
  VkDescriptorPoolCreateInfo info{};
  info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
  info.poolSizeCount = lengthof(sizes);
  info.pPoolSizes = sizes;
  info.maxSets = 16; // number of desc sets that can be allocated from this pool

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

VkPipeline createColorPipeline(VkDevice device, VkPipelineLayout pipelineLayout, VkExtent2D swapchainExtent, VkRenderPass renderPass)
{
  auto vertShaderCode = loadFile("bin/src/bloom/shader.vert.spv");
  auto fragShaderCode = loadFile("bin/src/bloom/shader.frag.spv");

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
  viewport.height = -(float)swapchainExtent.height;
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

  VkPipelineColorBlendStateCreateInfo colorBlending{};
  colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
  colorBlending.logicOpEnable = VK_FALSE;
  colorBlending.logicOp = VK_LOGIC_OP_COPY;
  colorBlending.attachmentCount = 1;
  colorBlending.pAttachments = &colorBlendAttachment;

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

  VkPipeline pipeline{};

  if(vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &pipeline) != VK_SUCCESS)
    throw std::runtime_error("failed to create graphics pipeline");

  vkDestroyShaderModule(device, fragShaderModule, nullptr);
  vkDestroyShaderModule(device, vertShaderModule, nullptr);

  return pipeline;
}

VkPipeline createPostprocPipeline(VkDevice device, VkPipelineLayout pipelineLayout, VkExtent2D swapchainExtent, VkRenderPass renderPass, const char* shaderPath)
{
  auto vertShaderCode = loadFile("bin/src/bloom/quad.vert.spv");
  auto fragShaderCode = loadFile(shaderPath);

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
  viewport.height = -(float)swapchainExtent.height;
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

  VkPipelineColorBlendStateCreateInfo colorBlending{};
  colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
  colorBlending.logicOpEnable = VK_FALSE;
  colorBlending.logicOp = VK_LOGIC_OP_COPY;
  colorBlending.attachmentCount = 1;
  colorBlending.pAttachments = &colorBlendAttachment;

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
  VkDescriptorSet result;
  vkAllocateDescriptorSets(device, &allocateInfo, &result);
  assert(result);
  return result;
}

VulkanTexture createHdrOffscreenBuffer(VkDevice device, VkPhysicalDevice physicalDevice, VkExtent2D extent, VkRenderPass renderPass)
{
  VulkanTexture result{};

  // Create image for the HDR buffer
  {
    VkImageCreateInfo info{};
    info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    info.imageType = VK_IMAGE_TYPE_2D;
    info.extent = {extent.width, extent.height, 1};
    info.format = HdrFormat;
    info.mipLevels = 1;
    info.arrayLayers = 1;
    info.samples = VK_SAMPLE_COUNT_1_BIT;
    info.tiling = VK_IMAGE_TILING_OPTIMAL;
    info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    info.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
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
    info.format = HdrFormat;
    info.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};
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
    info.width = extent.width;
    info.height = extent.height;
    info.layers = 1;

    vkCreateFramebuffer(device, &info, nullptr, &result.framebuffer);
    assert(result.framebuffer);
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

VkRenderPass createColorRenderPass(VkDevice device)
{
  VkAttachmentDescription attachmentDescription{};
  attachmentDescription.format = HdrFormat;
  attachmentDescription.samples = VK_SAMPLE_COUNT_1_BIT;
  attachmentDescription.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
  attachmentDescription.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
  attachmentDescription.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
  attachmentDescription.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
  attachmentDescription.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  attachmentDescription.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

  VkAttachmentReference colorReference = {};
  colorReference.attachment = 0;
  colorReference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

  VkSubpassDescription subpass = {};
  subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
  subpass.colorAttachmentCount = 1;
  subpass.pColorAttachments = &colorReference;

  // Use subpass dependencies for layout transitions
  VkSubpassDependency dependencies[2]{};

  dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
  dependencies[0].dstSubpass = 0;
  dependencies[0].srcStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
  dependencies[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
  dependencies[0].srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
  dependencies[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
  dependencies[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

  dependencies[1].srcSubpass = 0;
  dependencies[1].dstSubpass = VK_SUBPASS_EXTERNAL;
  dependencies[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
  dependencies[1].dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
  dependencies[1].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
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

VkRenderPass createPostProcRenderPass(VkDevice device)
{
  VkAttachmentDescription attachmentDescription{};
  attachmentDescription.format = HdrFormat;
  attachmentDescription.samples = VK_SAMPLE_COUNT_1_BIT;
  attachmentDescription.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
  attachmentDescription.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
  attachmentDescription.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
  attachmentDescription.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
  attachmentDescription.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  attachmentDescription.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

  VkAttachmentReference colorReference = {};
  colorReference.attachment = 0;
  colorReference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

  VkSubpassDescription subpass = {};
  subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
  subpass.colorAttachmentCount = 1;
  subpass.pColorAttachments = &colorReference;

  // Use subpass dependencies for layout transitions
  VkSubpassDependency dependencies[2]{};

  dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
  dependencies[0].dstSubpass = 0;
  dependencies[0].srcStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
  dependencies[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
  dependencies[0].srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
  dependencies[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
  dependencies[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

  dependencies[1].srcSubpass = 0;
  dependencies[1].dstSubpass = VK_SUBPASS_EXTERNAL;
  dependencies[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
  dependencies[1].dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
  dependencies[1].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
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

class Bloom : public IApp
{
public:
  Bloom(const AppCreationContext& ctx_)
      : ctx(ctx_)
  {
    descriptorPool = createDescriptorPool(ctx.device);
    descriptorSetLayout = createDescriptorSetLayout(ctx.device);

    colorRenderPass = createColorRenderPass(ctx.device);
    postprocRenderPass = createPostProcRenderPass(ctx.device);

    pipelineLayout = createPipelineLayout(ctx.device, descriptorSetLayout);

    colorPipeline = createColorPipeline(ctx.device, pipelineLayout, ctx.swapchainExtent, colorRenderPass);
    thresholdPipeline = createPostprocPipeline(ctx.device, pipelineLayout, ctx.swapchainExtent, postprocRenderPass, "bin/src/bloom/threshold.frag.spv");
    horzBlurPipeline = createPostprocPipeline(ctx.device, pipelineLayout, ctx.swapchainExtent, postprocRenderPass, "bin/src/bloom/horzblur.frag.spv");
    vertBlurPipeline = createPostprocPipeline(ctx.device, pipelineLayout, ctx.swapchainExtent, postprocRenderPass, "bin/src/bloom/vertblur.frag.spv");
    tonemapPipeline = createPostprocPipeline(ctx.device, pipelineLayout, ctx.swapchainExtent, ctx.renderPass, "bin/src/bloom/tonemapping.frag.spv");

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

    hdrBuffer[0] = createHdrOffscreenBuffer(ctx.device, ctx.physicalDevice, ctx.swapchainExtent, colorRenderPass);
    hdrBuffer[1] = createHdrOffscreenBuffer(ctx.device, ctx.physicalDevice, ctx.swapchainExtent, postprocRenderPass);
    hdrBuffer[2] = createHdrOffscreenBuffer(ctx.device, ctx.physicalDevice, ctx.swapchainExtent, postprocRenderPass);

    // associate descriptor sets and buffers
    descriptorSet[0] = createDescriptorSet(ctx.device, descriptorPool, descriptorSetLayout);
    setupDescriptorSet(ctx.device, descriptorSet[0], {hdrBuffer[0]}, uniformBuffer);

    descriptorSet[1] = createDescriptorSet(ctx.device, descriptorPool, descriptorSetLayout);
    setupDescriptorSet(ctx.device, descriptorSet[1], {hdrBuffer[1]}, uniformBuffer);

    descriptorSet[2] = createDescriptorSet(ctx.device, descriptorPool, descriptorSetLayout);
    setupDescriptorSet(ctx.device, descriptorSet[2], {hdrBuffer[2]}, uniformBuffer);

    descriptorSet[3] = createDescriptorSet(ctx.device, descriptorPool, descriptorSetLayout);
    setupDescriptorSet(ctx.device, descriptorSet[3], {hdrBuffer[0], hdrBuffer[1]}, uniformBuffer);
  }

  ~Bloom()
  {
    for(auto& buf : hdrBuffer)
      destroyTexture(ctx.device, buf);

    vkDestroyRenderPass(ctx.device, colorRenderPass, nullptr);
    vkDestroyRenderPass(ctx.device, postprocRenderPass, nullptr);

    vkDestroyBuffer(ctx.device, uniformBuffer, nullptr);
    vkFreeMemory(ctx.device, uniformBufferMemory, nullptr);
    vkDestroyBuffer(ctx.device, vertexBuffer, nullptr);
    vkFreeMemory(ctx.device, vertexBufferMemory, nullptr);
    vkDestroyPipeline(ctx.device, colorPipeline, nullptr);
    vkDestroyPipeline(ctx.device, thresholdPipeline, nullptr);
    vkDestroyPipeline(ctx.device, horzBlurPipeline, nullptr);
    vkDestroyPipeline(ctx.device, vertBlurPipeline, nullptr);
    vkDestroyPipeline(ctx.device, tonemapPipeline, nullptr);
    vkDestroyPipelineLayout(ctx.device, pipelineLayout, nullptr);

    vkDestroyDescriptorSetLayout(ctx.device, descriptorSetLayout, nullptr);
    vkDestroyDescriptorPool(ctx.device, descriptorPool, nullptr);
  }

  Camera m_camera;

  void setCamera(const Camera& camera) override { m_camera = camera; }

  void drawFrame(double time, VkFramebuffer framebuffer, VkCommandBuffer commandBuffer) override
  {
    // Color render pass: write to hdrBuffer[0]
    {
      VkClearValue clearColor{};

      VkRenderPassBeginInfo renderPassInfo{};
      renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
      renderPassInfo.renderPass = colorRenderPass;
      renderPassInfo.framebuffer = hdrBuffer[0].framebuffer;
      renderPassInfo.renderArea.offset = {0, 0};
      renderPassInfo.renderArea.extent = ctx.swapchainExtent;
      renderPassInfo.clearValueCount = 1;
      renderPassInfo.pClearValues = &clearColor;

      vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

      vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, colorPipeline);

      VkBuffer vertexBuffers[] = {vertexBuffer};
      VkDeviceSize offsets[] = {0};
      vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);

      vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &descriptorSet[0], 0, nullptr);

      MyUniformBlock constants{};
      const float angle = time * 3.5;

      constants.model = rotateZ(angle * 0.3) * rotateY(angle * 0.2) * rotateX(angle * 0.25);
      constants.view = m_camera.mat;
      constants.proj = perspective(1.5, 4.0 / 3.0, 0.1, 100);

      // convert row-major (app) to column-major (GLSL)
      constants.model = transpose(constants.model);
      constants.view = transpose(constants.view);
      constants.proj = transpose(constants.proj);

      writeToGpuMemory(ctx.device, uniformBufferMemory, &constants, sizeof constants);

      vkCmdDraw(commandBuffer, lengthof(vertices), 1, 0, 0);

      vkCmdEndRenderPass(commandBuffer);
    }

    // threshold render pass: read from hdrBuffer[0], write to hdrBuffer[1]
    {
      VkRenderPassBeginInfo renderPassInfo{};
      renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
      renderPassInfo.renderPass = postprocRenderPass;
      renderPassInfo.framebuffer = hdrBuffer[1].framebuffer;
      renderPassInfo.renderArea.extent = ctx.swapchainExtent;

      vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
      vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, thresholdPipeline);
      vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &descriptorSet[0], 0, nullptr);

      vkCmdDraw(commandBuffer, 6, 1, 0, 0);
      vkCmdEndRenderPass(commandBuffer);
    }

    for(int k = 0; k < 4; ++k)
    {
      // Horz blur render pass: read from hdrBuffer[1], write to hdrBuffer[2]
      {
        VkRenderPassBeginInfo renderPassInfo{};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        renderPassInfo.renderPass = postprocRenderPass;
        renderPassInfo.framebuffer = hdrBuffer[2].framebuffer;
        renderPassInfo.renderArea.extent = ctx.swapchainExtent;

        vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, horzBlurPipeline);
        vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &descriptorSet[1], 0, nullptr);

        vkCmdDraw(commandBuffer, 6, 1, 0, 0);
        vkCmdEndRenderPass(commandBuffer);
      }

      // Vert blur render pass: read from hdrBuffer[2], write to hdrBuffer[1]
      {
        VkRenderPassBeginInfo renderPassInfo{};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        renderPassInfo.renderPass = postprocRenderPass;
        renderPassInfo.framebuffer = hdrBuffer[1].framebuffer;
        renderPassInfo.renderArea.extent = ctx.swapchainExtent;

        vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, vertBlurPipeline);
        vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &descriptorSet[2], 0, nullptr);

        vkCmdDraw(commandBuffer, 6, 1, 0, 0);
        vkCmdEndRenderPass(commandBuffer);
      }
    }

    // Tone-mapping render pass: read from hdrBuffer[0] and hdrBuffer[1], write to the swapchain framebuffer
    {
      VkClearValue clearColor{};

      VkRenderPassBeginInfo renderPassInfo{};
      renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
      renderPassInfo.renderPass = ctx.renderPass;
      renderPassInfo.framebuffer = framebuffer;
      renderPassInfo.renderArea.extent = ctx.swapchainExtent;
      renderPassInfo.clearValueCount = 1;
      renderPassInfo.pClearValues = &clearColor;

      vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

      vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, tonemapPipeline);
      vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &descriptorSet[3], 0, nullptr);

      vkCmdDraw(commandBuffer, 6, 1, 0, 0);

      vkCmdEndRenderPass(commandBuffer);
    }
  }

private:
  VkPipelineLayout pipelineLayout{};

  VkPipeline colorPipeline{};
  VkPipeline thresholdPipeline{};
  VkPipeline vertBlurPipeline{};
  VkPipeline horzBlurPipeline{};
  VkPipeline tonemapPipeline{};

  VkBuffer vertexBuffer{};
  VkDeviceMemory vertexBufferMemory{};
  VkDescriptorSetLayout descriptorSetLayout{};
  VkDescriptorPool descriptorPool{};
  VkDescriptorSet descriptorSet[4]{};
  VkBuffer uniformBuffer{};
  VkDeviceMemory uniformBufferMemory{};
  VulkanTexture hdrBuffer[3]{};

  VkRenderPass colorRenderPass{};
  VkRenderPass postprocRenderPass{};

  const AppCreationContext ctx;
};
} // namespace

REGISTER_APP(Bloom);
