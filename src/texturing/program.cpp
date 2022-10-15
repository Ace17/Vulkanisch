#include "common/app.h"
#include "common/util.h"
#include "common/vkutil.h"

#include <stdexcept>
#include <vector>

namespace
{
///////////////////////////////////////////////////////////////////////////////
// Vertex

struct Vertex
{
  float x, y;
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
            .format = VK_FORMAT_R32G32_SFLOAT,
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
      {+0.5f, -0.5f, /**/ 0, 2}, //
      {+0.5f, +0.5f, /**/ 0, 0}, //
      {-0.5f, +0.5f, /**/ 2, 0}, //
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

  // Samplers : 1 total
  sizes[0].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  sizes[0].descriptorCount = 1;

  // Uniform buffers : 1 total
  sizes[1].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
  sizes[1].descriptorCount = 1;

  // Create the global descriptor pool
  VkDescriptorPoolCreateInfo info{};
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
  auto vertShaderCode = loadFile("bin/src/texturing/shader.vert.spv");
  auto fragShaderCode = loadFile("bin/src/texturing/shader.frag.spv");

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

static int findTransferQueue(VkPhysicalDevice device)
{
  uint32_t queueFamilyCount = 0;
  vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

  std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
  vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

  int i = 0;

  for(const auto& queueFamily : queueFamilies)
  {
    if(queueFamily.queueFlags & VK_QUEUE_TRANSFER_BIT)
      return i;
  }

  return 0;
}

void setImageLayout(VkCommandBuffer cmdbuffer,
      VkImage image,
      VkImageLayout oldImageLayout,
      VkImageLayout newImageLayout,
      VkImageSubresourceRange subresourceRange)
{
  VkImageMemoryBarrier imageMemoryBarrier{};
  imageMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
  imageMemoryBarrier.oldLayout = oldImageLayout;
  imageMemoryBarrier.newLayout = newImageLayout;
  imageMemoryBarrier.image = image;
  imageMemoryBarrier.subresourceRange = subresourceRange;

  // Make sure any writes to the image have been finished
  imageMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
  imageMemoryBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

  vkCmdPipelineBarrier(cmdbuffer, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, 0, 0, nullptr, 0, nullptr, 1, &imageMemoryBarrier);
}

struct VulkanTexture
{
  VkImage image;
  VkImageView view;
  VkDeviceMemory memory;
  VkSampler sampler;
};

VulkanTexture createTexture(VkDevice device, VkPhysicalDevice physicalDevice, void* srcPixels, int width, int height)
{
  const int bufferSize = width * height * sizeof(float) * 4;

  // Create a host-visible staging buffer that contains the raw image data
  VkBuffer stagingBuffer;
  VkDeviceMemory stagingMemory;

  // This buffer is used as a transfer source for the buffer copy
  {
    {
      VkBufferCreateInfo info{};
      info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
      info.size = bufferSize;
      info.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
      info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

      vkCreateBuffer(device, &info, nullptr, &stagingBuffer);
    }

    // Get memory requirements for the staging buffer (alignment, memory type bits)
    VkMemoryRequirements memReqs;
    vkGetBufferMemoryRequirements(device, stagingBuffer, &memReqs);

    // Allocate memory for the staging buffer
    {
      VkMemoryAllocateInfo memAllocInfo{};
      memAllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
      memAllocInfo.allocationSize = memReqs.size;
      memAllocInfo.memoryTypeIndex =
            findMemoryType(physicalDevice, memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

      vkAllocateMemory(device, &memAllocInfo, nullptr, &stagingMemory);
    }

    vkBindBufferMemory(device, stagingBuffer, stagingMemory, 0);
  }

  // Copy texture data into staging buffer
  writeToGpuMemory(device, stagingMemory, srcPixels, bufferSize);

  auto const format = VK_FORMAT_R32G32B32A32_SFLOAT;

  VulkanTexture texture{};

  // Create optimal tiled target image
  {
    VkImageCreateInfo info{};
    info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    info.imageType = VK_IMAGE_TYPE_2D;
    info.format = format;
    info.mipLevels = 1;
    info.arrayLayers = 1;
    info.samples = VK_SAMPLE_COUNT_1_BIT;
    info.tiling = VK_IMAGE_TILING_OPTIMAL;
    info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    info.extent = {(uint32_t)width, (uint32_t)height, 1};
    info.usage = VK_IMAGE_USAGE_SAMPLED_BIT;
    // Ensure that the TRANSFER_DST bit is set for staging
    info.usage |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    vkCreateImage(device, &info, nullptr, &texture.image);
  }

  // Allocate memory for the texture image
  {
    {
      VkMemoryRequirements memReqs;
      vkGetImageMemoryRequirements(device, texture.image, &memReqs);

      VkMemoryAllocateInfo info{};
      info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
      info.allocationSize = memReqs.size;
      info.memoryTypeIndex = findMemoryType(physicalDevice, memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
      vkAllocateMemory(device, &info, nullptr, &texture.memory);
    }

    vkBindImageMemory(device, texture.image, texture.memory, 0);
  }

  auto TransferDataFromStagingBufferToTexture = [&](VkCommandBuffer cmdBuf) {
    // Image barrier for optimal image (target)
    // Optimal image will be used as destination for the copy
    VkImageSubresourceRange range{};
    range.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    range.baseMipLevel = 0;
    range.levelCount = 1;
    range.layerCount = 1;

    setImageLayout(cmdBuf, texture.image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, range);

    // Copy data from staging buffer
    VkBufferImageCopy region{};
    region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    region.imageSubresource.mipLevel = 0;
    region.imageSubresource.baseArrayLayer = 0;
    region.imageSubresource.layerCount = 1;
    region.imageExtent.width = width;
    region.imageExtent.height = height;
    region.imageExtent.depth = 1;
    region.bufferOffset = 0;

    vkCmdCopyBufferToImage(cmdBuf, stagingBuffer, texture.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

    setImageLayout(cmdBuf, texture.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, range);
  };

  const int transferQueue = findTransferQueue(physicalDevice);
  executeOneShotCommandBufferOnQueue(device, TransferDataFromStagingBufferToTexture, transferQueue);

  // Clean up staging resources
  vkFreeMemory(device, stagingMemory, nullptr);
  vkDestroyBuffer(device, stagingBuffer, nullptr);

  // Create sampler
  VkSamplerCreateInfo samplerCreateInfo{};
  samplerCreateInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
  samplerCreateInfo.magFilter = VK_FILTER_LINEAR;
  samplerCreateInfo.minFilter = VK_FILTER_LINEAR;
  samplerCreateInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
  samplerCreateInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
  samplerCreateInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
  samplerCreateInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
  samplerCreateInfo.mipLodBias = 0.0f;
  samplerCreateInfo.compareOp = VK_COMPARE_OP_NEVER;
  samplerCreateInfo.minLod = 0.0f;
  samplerCreateInfo.maxLod = 0.0f;
  samplerCreateInfo.maxAnisotropy = 1.0f;
  vkCreateSampler(device, &samplerCreateInfo, nullptr, &texture.sampler);

  // Create image view
  VkImageViewCreateInfo viewCreateInfo{};
  viewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
  viewCreateInfo.pNext = NULL;
  viewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
  viewCreateInfo.format = format;
  viewCreateInfo.components = {VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_A};
  viewCreateInfo.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};
  viewCreateInfo.subresourceRange.levelCount = 1;
  viewCreateInfo.image = texture.image;
  vkCreateImageView(device, &viewCreateInfo, nullptr, &texture.view);

  return texture;
}

void destroyTexture(VkDevice device, const VulkanTexture& texture)
{
  vkDestroyImageView(device, texture.view, nullptr);
  vkDestroySampler(device, texture.sampler, nullptr);
  vkDestroyImage(device, texture.image, nullptr);
  vkFreeMemory(device, texture.memory, nullptr);
}

struct MyUniformBlock
{
  float angle;
};

class Texturing : public IApp
{
public:
  Texturing(const AppCreationContext& ctx_)
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

    // Create the texture image
    struct Pixel
    {
      float r, g, b, a;
    };

    const int N = 128;
    const int period = N / 4;
    Pixel tex[N][N]{};
    for(int y = 0; y < N; ++y)
    {
      for(int x = 0; x < N; ++x)
      {
        tex[y][x].a = 1;
        if((x / period) % 2 == (y / period) % 2)
        {
          tex[y][x].r = 1;
          tex[y][x].g = 1;
          tex[y][x].b = 1;
        }
        else
        {
          tex[y][x].r = 1;
          tex[y][x].g = 0;
          tex[y][x].b = 0;
        }
      }
    }

    texture = createTexture(ctx.device, ctx.physicalDevice, tex, N, N);

    // associate descriptor sets and buffers
    {
      VkDescriptorImageInfo info{};
      info.sampler = texture.sampler;
      info.imageView = texture.view;
      info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

      // Binding 0: Sampler
      VkWriteDescriptorSet writeDescriptorSets[1]{};
      writeDescriptorSets[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
      writeDescriptorSets[0].dstSet = descriptorSet;
      writeDescriptorSets[0].dstBinding = 0;
      writeDescriptorSets[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
      writeDescriptorSets[0].pImageInfo = &info;
      writeDescriptorSets[0].descriptorCount = 1;

      vkUpdateDescriptorSets(ctx.device, lengthof(writeDescriptorSets), writeDescriptorSets, 0, nullptr);
    }

    {
      VkDescriptorBufferInfo info{};
      info.buffer = uniformBuffer;
      info.range = sizeof(MyUniformBlock);

      // Binding 1: uniform buffer
      VkWriteDescriptorSet writeDescriptorSets[1]{};
      writeDescriptorSets[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
      writeDescriptorSets[0].dstSet = descriptorSet;
      writeDescriptorSets[0].dstBinding = 1;
      writeDescriptorSets[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
      writeDescriptorSets[0].pBufferInfo = &info;
      writeDescriptorSets[0].descriptorCount = 1;

      vkUpdateDescriptorSets(ctx.device, lengthof(writeDescriptorSets), writeDescriptorSets, 0, nullptr);
    }
  }

  ~Texturing()
  {
    destroyTexture(ctx.device, texture);

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
    (void)time;

    VkClearValue clearColor{};
    clearColor.color.float32[0] = 0.0f;
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

    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &descriptorSet, 0, nullptr);

    MyUniformBlock constants;
    constants.angle = time * 2;
    writeToGpuMemory(ctx.device, uniformBufferMemory, &constants, sizeof constants);

    vkCmdDraw(commandBuffer, 3, 1, 0, 0);

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
  VulkanTexture texture{};

  const AppCreationContext ctx;
};
} // namespace

REGISTER_APP(Texturing);
