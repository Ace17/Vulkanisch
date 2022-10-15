#include "common/app.h"
#include "common/util.h"
#include "common/vkutil.h"

#include <cmath> // sin
#include <stdexcept>
#include <vector>

namespace
{
///////////////////////////////////////////////////////////////////////////////
// Shader params

struct MyUniformBlock
{
  float cr, cg, cb, ca;
  float x, y;
};

///////////////////////////////////////////////////////////////////////////////
// Vertex

struct Vertex
{
  float x, y;
  float r, g, b;
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
            .format = VK_FORMAT_R32G32_SFLOAT,
            .offset = offsetof(Vertex, x),
      },
      // color
      {
            .location = 1,
            .binding = 0,
            .format = VK_FORMAT_R32G32B32_SFLOAT,
            .offset = offsetof(Vertex, r),
      }};

const Vertex vertices[] = {
      {-0.5f, -0.0f, /**/ 0, 0, 1}, //
      {-0.5f, -0.5f, /**/ 0, 0, 1}, //
      {-0.0f, -0.5f, /**/ 0, 0, 1}, //

      {+0.5f, +0.0f, /**/ 1, 0, 0}, //
      {+0.5f, +0.5f, /**/ 1, 0, 0}, //
      {+0.0f, +0.5f, /**/ 1, 0, 0}, //

      {+0.2f, +0.0f, /**/ 1, 1, 0}, //
      {+0.2f, +0.2f, /**/ 1, 1, 0}, //
      {+0.0f, +0.2f, /**/ 1, 1, 0}, //
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
  VkDescriptorSetLayoutBinding setLayoutBindings[1]{};

  // Binding 0: Uniform buffers
  setLayoutBindings[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
  setLayoutBindings[0].binding = 0;
  setLayoutBindings[0].stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
  setLayoutBindings[0].descriptorCount = 1;

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
  VkDescriptorPoolSize sizes[1];

  // Uniform buffers : 1 total
  sizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
  sizes[0].descriptorCount = 1;

  // Create the global descriptor pool
  VkDescriptorPoolCreateInfo info = {};
  info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
  info.poolSizeCount = lengthof(sizes);
  info.pPoolSizes = sizes;
  info.maxSets = 1; // number of desc sets that can be allocated from this pool

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

VkPipeline createGraphicsPipeline(VkDevice device, VkPipelineLayout pipelineLayout, VkExtent2D swapchainExtent, VkRenderPass renderPass)
{
  auto vertShaderCode = loadFile("bin/src/descriptor-sets/shader.vert.spv");
  auto fragShaderCode = loadFile("bin/src/descriptor-sets/shader.frag.spv");

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
  viewport.y = 0.0f;
  viewport.width = (float)swapchainExtent.width;
  viewport.height = (float)swapchainExtent.height;
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
  rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;
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
  pipelineInfo.stageCount = 2;
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

class DescriptorSets : public IApp
{
public:
  DescriptorSets(const AppCreationContext& ctx_)
      : ctx(ctx_)
  {
    descriptorPool = createDescriptorPool(ctx.device);
    descriptorSetLayout = createDescriptorSetLayout(ctx.device);
    descriptorSet = createDescriptorSet(ctx.device, descriptorPool, descriptorSetLayout);

    pipelineLayout = createPipelineLayout(ctx.device, descriptorSetLayout);
    graphicsPipeline = createGraphicsPipeline(ctx.device, pipelineLayout, ctx.swapchainExtent, ctx.renderPass);

    // Create the vertex buffer and send it to the GPU
    vertexBuffer = createVertexBuffer(ctx.device, lengthof(vertices) * sizeof(vertices[0]));
    vertexBufferMemory = createBufferMemory(ctx.physicalDevice, ctx.device, vertexBuffer);
    writeToGpuMemory(ctx.device, vertexBufferMemory, vertices, lengthof(vertices) * sizeof(vertices[0]));

    // Create the buffer for the uniform buffer
    {
      VkBufferCreateInfo bufferInfo{};
      bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
      bufferInfo.size = sizeof(MyUniformBlock);
      bufferInfo.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
      bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
      if(vkCreateBuffer(ctx.device, &bufferInfo, nullptr, &uniformBuffer) != VK_SUCCESS)
        throw std::runtime_error("failed to create uniform buffer");
    }
    uniformBufferMemory = createBufferMemory(ctx.physicalDevice, ctx.device, uniformBuffer);

    // associate descriptor sets and buffers
    {
      VkDescriptorBufferInfo bufferInfo{};
      bufferInfo.buffer = uniformBuffer;
      bufferInfo.range = sizeof(MyUniformBlock);

      // Binding 0: MyUniformBlock
      VkWriteDescriptorSet writeDescriptorSets[1]{};
      writeDescriptorSets[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
      writeDescriptorSets[0].dstSet = descriptorSet;
      writeDescriptorSets[0].dstBinding = 0;
      writeDescriptorSets[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
      writeDescriptorSets[0].pBufferInfo = &bufferInfo;
      writeDescriptorSets[0].descriptorCount = 1;

      vkUpdateDescriptorSets(ctx.device, lengthof(writeDescriptorSets), writeDescriptorSets, 0, nullptr);
    }
  }

  ~DescriptorSets()
  {
    vkDestroyBuffer(ctx.device, uniformBuffer, nullptr);
    vkFreeMemory(ctx.device, uniformBufferMemory, nullptr);
    vkDestroyBuffer(ctx.device, vertexBuffer, nullptr);
    vkFreeMemory(ctx.device, vertexBufferMemory, nullptr);
    vkDestroyPipeline(ctx.device, graphicsPipeline, nullptr);
    vkDestroyPipelineLayout(ctx.device, pipelineLayout, nullptr);

    // vkFreeDescriptorSets(ctx.device, descriptorPool, 1, &descriptorSet);
    vkDestroyDescriptorSetLayout(ctx.device, descriptorSetLayout, nullptr);
    vkDestroyDescriptorPool(ctx.device, descriptorPool, nullptr);
  }

  void drawFrame(double time, VkFramebuffer framebuffer, VkCommandBuffer commandBuffer) override
  {
    VkClearValue clearColor{};
    clearColor.color.float32[0] = 1.0f;
    clearColor.color.float32[1] = 0.0f;
    clearColor.color.float32[2] = 0.0f;
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

    MyUniformBlock constants{};
    constants.x = sin(time * 0.2) * 0.5;
    constants.y = 0.1;
    constants.cr = sin(time * 2.0) * 0.5;

    writeToGpuMemory(ctx.device, uniformBufferMemory, &constants, sizeof constants);

    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &descriptorSet, 0, nullptr);

    vkCmdDraw(commandBuffer, 9, 1, 0, 0);

    vkCmdEndRenderPass(commandBuffer);
  }

private:
  VkPipelineLayout pipelineLayout{};
  VkPipeline graphicsPipeline{};
  VkBuffer vertexBuffer{};
  VkDeviceMemory vertexBufferMemory{};
  VkDescriptorSetLayout descriptorSetLayout{};
  VkDescriptorPool descriptorPool{};
  VkDescriptorSet descriptorSet{};
  VkBuffer uniformBuffer{};
  VkDeviceMemory uniformBufferMemory{};

  const AppCreationContext ctx;
};
} // namespace

REGISTER_APP(DescriptorSets);
