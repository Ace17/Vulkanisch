#include "common/app.h"
#include "common/matrix4.h"
#include "common/util.h"
#include "common/vkutil.h"

#include <cassert>
#include <stdexcept>
#include <vector>

#include "objloader.h"

namespace
{
const VkFormat DepthFormat = VK_FORMAT_D16_UNORM;
const VkFormat HdrFormat = VK_FORMAT_R32G32B32A32_SFLOAT;

const int ShadowMapSize = 4096;

///////////////////////////////////////////////////////////////////////////////
// Vertex

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
      }};

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

VkDescriptorPool createDescriptorPool(VkDevice device)
{
  VkDescriptorPoolSize sizes[2];

  // Uniform buffers
  sizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
  sizes[0].descriptorCount = 16;

  // Samplers
  sizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
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

VkPipelineLayout createPipelineLayout(VkDevice device, std::vector<VkDescriptorSetLayout> setLayouts)
{
  VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
  pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
  pipelineLayoutInfo.setLayoutCount = setLayouts.size();
  pipelineLayoutInfo.pSetLayouts = setLayouts.data();

  VkPipelineLayout pipelineLayout;

  if(vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &pipelineLayout) != VK_SUCCESS)
    throw std::runtime_error("failed to create pipeline layout");

  return pipelineLayout;
}

VkPipeline createShadowMapPipeline(VkDevice device, VkPipelineLayout pipelineLayout, VkRenderPass renderPass)
{
  auto vertShaderCode = loadFile("bin/src/fulldemo/shader.vert.spv");

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

VkPipeline createColorPipeline(VkDevice device, VkPipelineLayout pipelineLayout, VkExtent2D swapchainExtent, VkRenderPass renderPass)
{
  auto vertShaderCode = loadFile("bin/src/fulldemo/shader.vert.spv");
  auto fragShaderCode = loadFile("bin/src/fulldemo/shader.frag.spv");

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

VkPipeline createPostprocPipeline(VkDevice device, VkPipelineLayout pipelineLayout, VkExtent2D swapchainExtent, VkRenderPass renderPass, const char* shaderPath)
{
  auto vertShaderCode = loadFile("bin/src/fulldemo/quad.vert.spv");
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
  VkDescriptorSet descriptorSet;
  vkAllocateDescriptorSets(device, &allocateInfo, &descriptorSet);
  assert(descriptorSet);
  return descriptorSet;
}

struct VulkanMesh
{
  int material;
  int vertexCount = 0;
  VkBuffer vertexBuffer{};
  VkDeviceMemory vertexMemory{};
};

struct VulkanFramebuffer
{
  VkImage image;
  VkImageView view;
  VkDeviceMemory memory;
  VkSampler sampler;
  VkFramebuffer framebuffer;
};

struct VulkanFramebufferWithDepth : VulkanFramebuffer
{
  VkImage depthImage;
  VkImageView depthView;
  VkDeviceMemory depthMemory;
};

struct VulkanMaterial
{
  VkDescriptorSet descriptorSet;
  VkBuffer uniformBuffer;
  VkDeviceMemory memory;
};

struct MyUniformBlock
{
  Matrix4f model;
  Matrix4f view;
  Matrix4f proj;
  Matrix4f LightMVP;
};

struct MaterialParams
{
  Vec4f diffuse;
  Vec4f emissive;
};

// Perspective: Scene (set=0)
VkDescriptorSetLayout createSceneDescriptorSetLayout(VkDevice device)
{
  // Camera (binding=0)
  VkDescriptorSetLayoutBinding cameraBinding{};
  cameraBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
  cameraBinding.binding = 0;
  cameraBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
  cameraBinding.descriptorCount = 1;

  // Shadow Map (binding=1)
  VkDescriptorSetLayoutBinding shadowMapBinding{};
  shadowMapBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  shadowMapBinding.binding = 1;
  shadowMapBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
  shadowMapBinding.descriptorCount = 1;

  VkDescriptorSetLayoutBinding setLayoutBindings[] = {cameraBinding, shadowMapBinding};

  // Create the descriptor set layout
  VkDescriptorSetLayoutCreateInfo info{};
  info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
  info.bindingCount = lengthof(setLayoutBindings);
  info.pBindings = setLayoutBindings;

  VkDescriptorSetLayout result;
  vkCreateDescriptorSetLayout(device, &info, nullptr, &result);

  return result;
}

// Perspective: Scene (set=0)
void setupDescriptorSet_MainScene(VkDevice device, VkDescriptorSet ds, VkBuffer uniformBuffer, const VulkanFramebuffer& shadowMap)
{
  assert(ds);

  VkWriteDescriptorSet writeInfo[2]{};

  VkDescriptorBufferInfo infoBinding1{};
  infoBinding1.buffer = uniformBuffer;
  infoBinding1.range = sizeof(MyUniformBlock);

  // Binding 0: Camera
  writeInfo[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  writeInfo[0].dstSet = ds;
  writeInfo[0].dstBinding = 0;
  writeInfo[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
  writeInfo[0].pBufferInfo = &infoBinding1;
  writeInfo[0].descriptorCount = 1;

  VkDescriptorImageInfo infoBinding0{};
  infoBinding0.sampler = shadowMap.sampler;
  infoBinding0.imageView = shadowMap.view;
  infoBinding0.imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;

  // Binding 1: ShadowMap
  writeInfo[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  writeInfo[1].dstSet = ds;
  writeInfo[1].dstBinding = 1;
  writeInfo[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  writeInfo[1].pImageInfo = &infoBinding0;
  writeInfo[1].descriptorCount = 1;

  vkUpdateDescriptorSets(device, lengthof(writeInfo), writeInfo, 0, nullptr);
}

// Perspective: Scene (set=0)
void setupDescriptorSet_ShadowMapScene(VkDevice device, VkDescriptorSet ds, VkBuffer shadowMapUniformBuffer)
{
  VkWriteDescriptorSet writeInfo[1]{};

  VkDescriptorBufferInfo infoBinding1{};
  infoBinding1.buffer = shadowMapUniformBuffer;
  infoBinding1.range = sizeof(MyUniformBlock);

  // Binding 0: uniform buffer
  writeInfo[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  writeInfo[0].dstSet = ds;
  writeInfo[0].dstBinding = 0;
  writeInfo[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
  writeInfo[0].pBufferInfo = &infoBinding1;
  writeInfo[0].descriptorCount = 1;

  vkUpdateDescriptorSets(device, lengthof(writeInfo), writeInfo, 0, nullptr);
}

// Postproc (set=0)
VkDescriptorSetLayout createPostprocDescriptorSetLayout(VkDevice device)
{
  // InputPicture #0 (binding=0)
  VkDescriptorSetLayoutBinding tex0{};
  tex0.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  tex0.binding = 0;
  tex0.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
  tex0.descriptorCount = 1;

  // InputPicture #1 (binding=1)
  VkDescriptorSetLayoutBinding tex1{};
  tex1.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  tex1.binding = 1;
  tex1.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
  tex1.descriptorCount = 1;

  VkDescriptorSetLayoutBinding setLayoutBindings[] = {tex0, tex1};

  // Create the descriptor set layout
  VkDescriptorSetLayoutCreateInfo info{};
  info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
  info.bindingCount = lengthof(setLayoutBindings);
  info.pBindings = setLayoutBindings;

  VkDescriptorSetLayout result;
  vkCreateDescriptorSetLayout(device, &info, nullptr, &result);

  return result;
}

// PostProc (set=0)
void setupDescriptorSet_InputPicture(VkDevice device, VkDescriptorSet ds, VulkanFramebuffer& inputPicture0, VulkanFramebuffer& inputPicture1)
{
  assert(ds);

  VkWriteDescriptorSet writeInfo[2]{};

  VkDescriptorImageInfo infoBinding0{};
  infoBinding0.sampler = inputPicture0.sampler;
  infoBinding0.imageView = inputPicture0.view;
  infoBinding0.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

  // Binding 0: Input Picture 0
  writeInfo[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  writeInfo[0].dstSet = ds;
  writeInfo[0].dstBinding = 0;
  writeInfo[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  writeInfo[0].pImageInfo = &infoBinding0;
  writeInfo[0].descriptorCount = 1;

  VkDescriptorImageInfo infoBinding1{};
  infoBinding1.sampler = inputPicture1.sampler;
  infoBinding1.imageView = inputPicture1.view;
  infoBinding1.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

  // Binding 1: Input Picture 1
  writeInfo[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  writeInfo[1].dstSet = ds;
  writeInfo[1].dstBinding = 1;
  writeInfo[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  writeInfo[1].pImageInfo = &infoBinding1;
  writeInfo[1].descriptorCount = 1;

  vkUpdateDescriptorSets(device, lengthof(writeInfo), writeInfo, 0, nullptr);
}

// Material (set=1)
VkDescriptorSetLayout createMaterialDescriptorSetLayout(VkDevice device)
{
  // MaterialParams (binding=0)
  VkDescriptorSetLayoutBinding materialParamsBinding{};
  materialParamsBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
  materialParamsBinding.binding = 0;
  materialParamsBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
  materialParamsBinding.descriptorCount = 1;

  VkDescriptorSetLayoutBinding setLayoutBindings[] = {materialParamsBinding};

  // Create the descriptor set layout
  VkDescriptorSetLayoutCreateInfo info{};
  info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
  info.bindingCount = lengthof(setLayoutBindings);
  info.pBindings = setLayoutBindings;

  VkDescriptorSetLayout result;
  vkCreateDescriptorSetLayout(device, &info, nullptr, &result);

  return result;
}

// Material (set=1)
void setupDescriptorSet_Material(VkDevice device, VkDescriptorSet ds, VkBuffer uniformBuffer)
{
  VkWriteDescriptorSet writeInfo[1]{};

  VkDescriptorBufferInfo bufferInfo{};
  bufferInfo.buffer = uniformBuffer;
  bufferInfo.range = sizeof(MaterialParams);

  // Binding 0: uniform buffer for MaterialParams
  writeInfo[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  writeInfo[0].dstSet = ds;
  writeInfo[0].dstBinding = 0;
  writeInfo[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
  writeInfo[0].pBufferInfo = &bufferInfo;
  writeInfo[0].descriptorCount = 1;

  vkUpdateDescriptorSets(device, lengthof(writeInfo), writeInfo, 0, nullptr);
}

VulkanFramebuffer createShadowFramebuffer(VkDevice device, VkPhysicalDevice physicalDevice, int width, int height, VkRenderPass renderPass)
{
  VulkanFramebuffer result{};

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

VulkanFramebufferWithDepth createColorFramebuffer(VkDevice device, VkPhysicalDevice physicalDevice, VkExtent2D extent, VkRenderPass renderPass)
{
  VulkanFramebufferWithDepth result{};

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

  // Create image for the depth buffer
  {
    VkImageCreateInfo info{};
    info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    info.imageType = VK_IMAGE_TYPE_2D;
    info.extent = {extent.width, extent.height, 1};
    info.format = DepthFormat;
    info.mipLevels = 1;
    info.arrayLayers = 1;
    info.samples = VK_SAMPLE_COUNT_1_BIT;
    info.tiling = VK_IMAGE_TILING_OPTIMAL;
    info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    info.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    vkCreateImage(device, &info, nullptr, &result.depthImage);
  }

  // Allocate memory for the texture image
  {
    {
      VkMemoryRequirements memReqs;
      vkGetImageMemoryRequirements(device, result.depthImage, &memReqs);

      VkMemoryAllocateInfo info{};
      info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
      info.allocationSize = memReqs.size;
      info.memoryTypeIndex = findMemoryType(physicalDevice, memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
      vkAllocateMemory(device, &info, nullptr, &result.depthMemory);
    }

    vkBindImageMemory(device, result.depthImage, result.depthMemory, 0);
  }

  // Create image view
  {
    VkImageViewCreateInfo info{};
    info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    info.viewType = VK_IMAGE_VIEW_TYPE_2D;
    info.format = DepthFormat;
    info.subresourceRange = {VK_IMAGE_ASPECT_DEPTH_BIT, 0, 1, 0, 1};
    info.subresourceRange.levelCount = 1;
    info.image = result.depthImage;
    vkCreateImageView(device, &info, nullptr, &result.depthView);
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
    VkImageView views[] = {result.view, result.depthView};
    VkFramebufferCreateInfo info{};
    info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    info.renderPass = renderPass;
    info.attachmentCount = lengthof(views);
    info.pAttachments = views;
    info.width = extent.width;
    info.height = extent.height;
    info.layers = 1;

    vkCreateFramebuffer(device, &info, nullptr, &result.framebuffer);
    assert(result.framebuffer);
  }

  return result;
}

VulkanFramebuffer createHdrFramebuffer(VkDevice device, VkPhysicalDevice physicalDevice, VkExtent2D extent, VkRenderPass renderPass)
{
  VulkanFramebuffer result{};

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

void destroyTexture(VkDevice device, const VulkanFramebuffer& texture)
{
  vkDestroyFramebuffer(device, texture.framebuffer, nullptr);
  vkDestroyImageView(device, texture.view, nullptr);
  vkDestroySampler(device, texture.sampler, nullptr);
  vkDestroyImage(device, texture.image, nullptr);
  vkFreeMemory(device, texture.memory, nullptr);
}

void destroyTexture(VkDevice device, const VulkanFramebufferWithDepth& texture)
{
  const VulkanFramebuffer& buf = texture;
  destroyTexture(device, buf);

  vkDestroyImageView(device, texture.depthView, nullptr);
  vkDestroyImage(device, texture.depthImage, nullptr);
  vkFreeMemory(device, texture.depthMemory, nullptr);
}

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

  VkRenderPassCreateInfo info{};
  info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
  info.attachmentCount = 1;
  info.pAttachments = &attachmentDescription;
  info.subpassCount = 1;
  info.pSubpasses = &subpass;

  VkRenderPass renderPass{};
  vkCreateRenderPass(device, &info, nullptr, &renderPass);
  return renderPass;
}

VkRenderPass createColorRenderPass(VkDevice device)
{
  VkAttachmentDescription colorAttachment{};
  colorAttachment.format = HdrFormat;
  colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
  colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
  colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
  colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
  colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
  colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  colorAttachment.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

  VkAttachmentDescription depthAttachment{};
  depthAttachment.format = DepthFormat;
  depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
  depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
  depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
  depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
  depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
  depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;

  // will be used as color during render pass
  VkAttachmentReference colorReference = {};
  colorReference.attachment = 0;
  colorReference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

  // will be used as depth/stencil during render pass
  VkAttachmentReference depthReference = {};
  depthReference.attachment = 1;
  depthReference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

  VkSubpassDescription subpass = {};
  subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
  subpass.colorAttachmentCount = 1;
  subpass.pColorAttachments = &colorReference;
  subpass.pDepthStencilAttachment = &depthReference;

  VkAttachmentDescription attachments[2] = {colorAttachment, depthAttachment};

  VkRenderPassCreateInfo info{};
  info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
  info.attachmentCount = lengthof(attachments);
  info.pAttachments = attachments;
  info.subpassCount = 1;
  info.pSubpasses = &subpass;

  VkRenderPass renderPass{};
  vkCreateRenderPass(device, &info, nullptr, &renderPass);
  return renderPass;
}

VkRenderPass createPostprocRenderPass(VkDevice device)
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
  colorReference.layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

  VkSubpassDescription subpass = {};
  subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
  subpass.colorAttachmentCount = 1;
  subpass.pColorAttachments = &colorReference;

  VkRenderPassCreateInfo info{};
  info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
  info.attachmentCount = 1;
  info.pAttachments = &attachmentDescription;
  info.subpassCount = 1;
  info.pSubpasses = &subpass;

  VkRenderPass renderPass{};
  vkCreateRenderPass(device, &info, nullptr, &renderPass);
  return renderPass;
}

class FullDemo : public IApp
{
public:
  FullDemo(const AppCreationContext& ctx_)
      : ctx(ctx_)
  {
    shadowRenderPass = createShadowMapRenderPass(ctx.device);
    colorRenderPass = createColorRenderPass(ctx.device);
    postprocRenderPass = createPostprocRenderPass(ctx.device);

    sceneDescriptorSetLayout = createSceneDescriptorSetLayout(ctx.device);
    materialDescriptorSetLayout = createMaterialDescriptorSetLayout(ctx.device);
    postprocDescriptorSetLayout = createPostprocDescriptorSetLayout(ctx.device);

    perspectivePipelineLayout = createPipelineLayout(ctx.device, {sceneDescriptorSetLayout, materialDescriptorSetLayout});

    postprocPipelineLayout = createPipelineLayout(ctx.device, {postprocDescriptorSetLayout});

    shadowMapPipeline = createShadowMapPipeline(ctx.device, perspectivePipelineLayout, shadowRenderPass);
    colorPipeline = createColorPipeline(ctx.device, perspectivePipelineLayout, ctx.swapchainExtent, colorRenderPass);

    thresholdPipeline =
          createPostprocPipeline(ctx.device, postprocPipelineLayout, ctx.swapchainExtent, postprocRenderPass, "bin/src/fulldemo/threshold.frag.spv");
    horzBlurPipeline =
          createPostprocPipeline(ctx.device, postprocPipelineLayout, ctx.swapchainExtent, postprocRenderPass, "bin/src/fulldemo/horzblur.frag.spv");
    vertBlurPipeline =
          createPostprocPipeline(ctx.device, postprocPipelineLayout, ctx.swapchainExtent, postprocRenderPass, "bin/src/fulldemo/vertblur.frag.spv");
    tonemapPipeline = createPostprocPipeline(ctx.device, postprocPipelineLayout, ctx.swapchainExtent, ctx.renderPass, "bin/src/fulldemo/tonemapping.frag.spv");

    hdrBuffer = createColorFramebuffer(ctx.device, ctx.physicalDevice, ctx.swapchainExtent, colorRenderPass);
    bloomBuffer[0] = createHdrFramebuffer(ctx.device, ctx.physicalDevice, ctx.swapchainExtent, postprocRenderPass);
    bloomBuffer[1] = createHdrFramebuffer(ctx.device, ctx.physicalDevice, ctx.swapchainExtent, postprocRenderPass);

    shadowMap = createShadowFramebuffer(ctx.device, ctx.physicalDevice, ShadowMapSize, ShadowMapSize, shadowRenderPass);

    auto scene = loadObj("data/scifi-01.obj");

    descriptorPool = createDescriptorPool(ctx.device);

    for(auto& plainMesh : scene.plainMeshes)
    {
      auto& vertices = plainMesh.vertices;

      // Create the vertex buffer and send it to the GPU
      vulkanMeshes.push_back({});
      VulkanMesh& vulkanMesh = vulkanMeshes.back();
      vulkanMesh.material = plainMesh.material;
      vulkanMesh.vertexCount = vertices.size();
      vulkanMesh.vertexBuffer = createVertexBuffer(ctx.device, vertices.size() * sizeof(vertices[0]));
      vulkanMesh.vertexMemory = createBufferMemory(ctx.physicalDevice, ctx.device, vulkanMesh.vertexBuffer);
      writeToGpuMemory(ctx.device, vulkanMesh.vertexMemory, vertices.data(), vertices.size() * sizeof(vertices[0]));
    }

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

    // fill descriptor set for main scene
    mainSceneDescriptorSet = createDescriptorSet(ctx.device, descriptorPool, sceneDescriptorSetLayout);
    setupDescriptorSet_MainScene(ctx.device, mainSceneDescriptorSet, uniformBuffer, shadowMap);

    // fill descriptor set for shadowmap scene
    shadowMapDescriptorSet = createDescriptorSet(ctx.device, descriptorPool, sceneDescriptorSetLayout);
    setupDescriptorSet_ShadowMapScene(ctx.device, shadowMapDescriptorSet, shadowMapUniformBuffer);

    // fill descriptor sets for postproc pipelines
    postprocDescriptorSet_Hdr_And_Bloom0 = createDescriptorSet(ctx.device, descriptorPool, postprocDescriptorSetLayout);
    setupDescriptorSet_InputPicture(ctx.device, postprocDescriptorSet_Hdr_And_Bloom0, hdrBuffer, bloomBuffer[0]);

    postprocDescriptorSet_Bloom0_And_Bloom1 = createDescriptorSet(ctx.device, descriptorPool, postprocDescriptorSetLayout);
    setupDescriptorSet_InputPicture(ctx.device, postprocDescriptorSet_Bloom0_And_Bloom1, bloomBuffer[0], bloomBuffer[1]);

    for(auto& material : scene.materials)
    {
      vulkanMaterials.push_back({});
      auto& vulkanMaterial = vulkanMaterials.back();

      {
        VkBufferCreateInfo info{};
        info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        info.size = sizeof(MaterialParams);
        info.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
        info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        if(vkCreateBuffer(ctx.device, &info, nullptr, &vulkanMaterial.uniformBuffer) != VK_SUCCESS)
          throw std::runtime_error("failed to create uniform buffer");

        vulkanMaterial.memory = createBufferMemory(ctx.physicalDevice, ctx.device, vulkanMaterial.uniformBuffer);
      }

      {
        MaterialParams params{};
        params.diffuse.x = material.diffuse.r;
        params.diffuse.y = material.diffuse.g;
        params.diffuse.z = material.diffuse.b;
        params.emissive.x = material.emissive.r;
        params.emissive.y = material.emissive.g;
        params.emissive.z = material.emissive.b;

        writeToGpuMemory(ctx.device, vulkanMaterial.memory, &params, sizeof params);
      }

      vulkanMaterial.descriptorSet = createDescriptorSet(ctx.device, descriptorPool, materialDescriptorSetLayout);
      setupDescriptorSet_Material(ctx.device, vulkanMaterial.descriptorSet, vulkanMaterial.uniformBuffer);
    }
  }

  ~FullDemo()
  {
    destroyTexture(ctx.device, shadowMap);
    destroyTexture(ctx.device, hdrBuffer);
    destroyTexture(ctx.device, bloomBuffer[0]);
    destroyTexture(ctx.device, bloomBuffer[1]);

    vkDestroyBuffer(ctx.device, shadowMapUniformBuffer, nullptr);
    vkDestroyBuffer(ctx.device, uniformBuffer, nullptr);
    vkFreeMemory(ctx.device, shadowMapUniformBufferMemory, nullptr);
    vkFreeMemory(ctx.device, uniformBufferMemory, nullptr);

    for(auto& mesh : vulkanMeshes)
    {
      vkDestroyBuffer(ctx.device, mesh.vertexBuffer, nullptr);
      vkFreeMemory(ctx.device, mesh.vertexMemory, nullptr);
    }

    for(auto& vulkanMaterial : vulkanMaterials)
    {
      vkDestroyBuffer(ctx.device, vulkanMaterial.uniformBuffer, nullptr);
      vkFreeMemory(ctx.device, vulkanMaterial.memory, nullptr);
    }

    vkDestroyPipeline(ctx.device, shadowMapPipeline, nullptr);
    vkDestroyPipeline(ctx.device, colorPipeline, nullptr);
    vkDestroyPipeline(ctx.device, thresholdPipeline, nullptr);
    vkDestroyPipeline(ctx.device, horzBlurPipeline, nullptr);
    vkDestroyPipeline(ctx.device, vertBlurPipeline, nullptr);
    vkDestroyPipeline(ctx.device, tonemapPipeline, nullptr);

    vkDestroyPipelineLayout(ctx.device, perspectivePipelineLayout, nullptr);
    vkDestroyPipelineLayout(ctx.device, postprocPipelineLayout, nullptr);

    vkDestroyRenderPass(ctx.device, shadowRenderPass, nullptr);
    vkDestroyRenderPass(ctx.device, colorRenderPass, nullptr);
    vkDestroyRenderPass(ctx.device, postprocRenderPass, nullptr);

    vkDestroyDescriptorSetLayout(ctx.device, sceneDescriptorSetLayout, nullptr);
    vkDestroyDescriptorSetLayout(ctx.device, materialDescriptorSetLayout, nullptr);
    vkDestroyDescriptorSetLayout(ctx.device, postprocDescriptorSetLayout, nullptr);

    vkDestroyDescriptorPool(ctx.device, descriptorPool, nullptr);
  }

  Camera m_camera;

  void setCamera(const Camera& camera) override { m_camera = camera; }

  void drawShadowMap(VkCommandBuffer commandBuffer, VkFramebuffer target, const Matrix4f& model, const Matrix4f& lightView, const Matrix4f& lightProj)
  {
    VkClearValue clearDepth{};
    clearDepth.depthStencil = {1.0, 0};

    VkRenderPassBeginInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.renderPass = shadowRenderPass;
    renderPassInfo.framebuffer = target;
    renderPassInfo.renderArea.offset = {0, 0};
    renderPassInfo.renderArea.extent = {ShadowMapSize, ShadowMapSize};
    renderPassInfo.clearValueCount = 1;
    renderPassInfo.pClearValues = &clearDepth;

    vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, shadowMapPipeline);

    for(auto& mesh : vulkanMeshes)
    {
      VkBuffer vertexBuffers[] = {mesh.vertexBuffer};
      VkDeviceSize offsets[] = {0};
      vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);

      vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, perspectivePipelineLayout, 0, 1, &shadowMapDescriptorSet, 0, nullptr);

      MyUniformBlock constants{};
      constants.model = model;
      constants.view = lightView;
      constants.proj = lightProj;

      // convert row-major (app) to column-major (GLSL)
      constants.model = transpose(constants.model);
      constants.view = transpose(constants.view);
      constants.proj = transpose(constants.proj);

      writeToGpuMemory(ctx.device, shadowMapUniformBufferMemory, &constants, sizeof constants);

      vkCmdDraw(commandBuffer, mesh.vertexCount, 1, 0, 0);
    }

    vkCmdEndRenderPass(commandBuffer);
  }

  void drawMainScene(VkCommandBuffer commandBuffer, VkFramebuffer target, const Matrix4f& model, const Matrix4f& mvpLight)
  {
    VkClearValue clearValues[2]{};
    clearValues[0].color.float32[0] = 0.1f;
    clearValues[0].color.float32[1] = 0.1f;
    clearValues[0].color.float32[2] = 0.1f;
    clearValues[0].color.float32[3] = 1.0f;
    clearValues[1].depthStencil.depth = 1;
    clearValues[1].depthStencil.stencil = 0;

    VkRenderPassBeginInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.renderPass = colorRenderPass;
    renderPassInfo.framebuffer = target;
    renderPassInfo.renderArea.offset = {0, 0};
    renderPassInfo.renderArea.extent = ctx.swapchainExtent;
    renderPassInfo.clearValueCount = lengthof(clearValues);
    renderPassInfo.pClearValues = clearValues;

    vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, colorPipeline);

    for(auto& mesh : vulkanMeshes)
    {
      VkBuffer vertexBuffers[] = {mesh.vertexBuffer};
      VkDeviceSize offsets[] = {0};
      vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);

      vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, perspectivePipelineLayout, 0, 1, &mainSceneDescriptorSet, 0, nullptr);

      vkCmdBindDescriptorSets(
            commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, perspectivePipelineLayout, 1, 1, &vulkanMaterials[mesh.material].descriptorSet, 0, nullptr);

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

      vkCmdDraw(commandBuffer, mesh.vertexCount, 1, 0, 0);
    }

    vkCmdEndRenderPass(commandBuffer);
  }

  void drawFrame(double time, VkFramebuffer swapchainFramebuffer, VkCommandBuffer commandBuffer) override
  {
    const float angle = time * 1.2;
    const Matrix4f model = rotateZ(angle * 0.3);

    const Matrix4f lightView = lookAt({6, 2, 7}, {}, {0, 0, 1});
    const Matrix4f lightProj = perspective(1.5, 1, 1, 100);
    const Matrix4f mvpLight = lightProj * lightView * model;

    drawShadowMap(commandBuffer, shadowMap.framebuffer, model, lightView, lightProj);
    drawMainScene(commandBuffer, hdrBuffer.framebuffer, model, mvpLight);

    // threshold render pass: read from hdrBuffer, write to bloomBuffer[0]
    {
      VkRenderPassBeginInfo renderPassInfo{};
      renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
      renderPassInfo.renderPass = postprocRenderPass;
      renderPassInfo.framebuffer = bloomBuffer[0].framebuffer;
      renderPassInfo.renderArea.extent = ctx.swapchainExtent;

      vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
      vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, thresholdPipeline);
      vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, postprocPipelineLayout, 0, 1, &postprocDescriptorSet_Hdr_And_Bloom0, 0, nullptr);

      vkCmdDraw(commandBuffer, 6, 1, 0, 0);
      vkCmdEndRenderPass(commandBuffer);
    }

    for(int k = 0; k < 4; ++k)
    {
      // Horz blur render pass: read from bloomBuffer[0], write to bloomBuffer[1]
      {
        VkRenderPassBeginInfo renderPassInfo{};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        renderPassInfo.renderPass = postprocRenderPass;
        renderPassInfo.framebuffer = bloomBuffer[1].framebuffer;
        renderPassInfo.renderArea.extent = ctx.swapchainExtent;

        vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, horzBlurPipeline);
        vkCmdBindDescriptorSets(
              commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, postprocPipelineLayout, 0, 1, &postprocDescriptorSet_Bloom0_And_Bloom1, 0, nullptr);

        vkCmdDraw(commandBuffer, 6, 1, 0, 0);
        vkCmdEndRenderPass(commandBuffer);
      }

      // Vert blur render pass: read from bloomBuffer[1], write to bloomBuffer[0]
      {
        VkRenderPassBeginInfo renderPassInfo{};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        renderPassInfo.renderPass = postprocRenderPass;
        renderPassInfo.framebuffer = bloomBuffer[0].framebuffer;
        renderPassInfo.renderArea.extent = ctx.swapchainExtent;

        vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, vertBlurPipeline);
        vkCmdBindDescriptorSets(
              commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, postprocPipelineLayout, 0, 1, &postprocDescriptorSet_Bloom0_And_Bloom1, 0, nullptr);

        vkCmdDraw(commandBuffer, 6, 1, 0, 0);
        vkCmdEndRenderPass(commandBuffer);
      }
    }

    // Tone-mapping render pass: read from hdrBuffer + bloomBuffer[0], write to the swapchain framebuffer
    {
      VkClearValue clearColor{};

      VkRenderPassBeginInfo renderPassInfo{};
      renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
      renderPassInfo.renderPass = ctx.renderPass;
      renderPassInfo.framebuffer = swapchainFramebuffer;
      renderPassInfo.renderArea.extent = ctx.swapchainExtent;
      renderPassInfo.clearValueCount = 1;
      renderPassInfo.pClearValues = &clearColor;

      vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

      vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, tonemapPipeline);
      vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, postprocPipelineLayout, 0, 1, &postprocDescriptorSet_Hdr_And_Bloom0, 0, nullptr);

      vkCmdDraw(commandBuffer, 6, 1, 0, 0);

      vkCmdEndRenderPass(commandBuffer);
    }
  }

private:
  VkPipelineLayout perspectivePipelineLayout{};
  VkPipelineLayout postprocPipelineLayout{};

  VkPipeline shadowMapPipeline{};
  VkPipeline colorPipeline{};
  VkPipeline thresholdPipeline{};
  VkPipeline vertBlurPipeline{};
  VkPipeline horzBlurPipeline{};
  VkPipeline tonemapPipeline{};

  std::vector<VulkanMesh> vulkanMeshes;
  std::vector<VulkanMaterial> vulkanMaterials;

  VkDescriptorSetLayout sceneDescriptorSetLayout{};
  VkDescriptorSetLayout materialDescriptorSetLayout{};
  VkDescriptorSetLayout postprocDescriptorSetLayout{};

  VkDescriptorPool descriptorPool{};
  VkDescriptorSet mainSceneDescriptorSet{};
  VkDescriptorSet shadowMapDescriptorSet{};
  VkDescriptorSet postprocDescriptorSet_Hdr_And_Bloom0{};
  VkDescriptorSet postprocDescriptorSet_Bloom0_And_Bloom1{};
  VkBuffer uniformBuffer{};
  VkBuffer shadowMapUniformBuffer{};
  VkDeviceMemory uniformBufferMemory{};
  VkDeviceMemory shadowMapUniformBufferMemory{};
  VulkanFramebuffer shadowMap{};

  VkRenderPass shadowRenderPass{};
  VkRenderPass colorRenderPass{};
  VkRenderPass postprocRenderPass{};

  VulkanFramebufferWithDepth hdrBuffer{};
  VulkanFramebuffer bloomBuffer[2]{};

  const AppCreationContext ctx;
};
} // namespace

REGISTER_APP(FullDemo);
