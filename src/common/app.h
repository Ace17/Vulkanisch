#pragma once

#include "glad/vulkan.h"

#include "matrix4.h"

///////////////////////////////////////////////////////////////////////////////
// Demo app

struct Camera
{
  Matrix4f mat;
};

struct IApp
{
  virtual ~IApp() = default;
  virtual void drawFrame(double time, VkFramebuffer framebuffer, VkCommandBuffer commandBuffer) = 0;
  virtual void setCamera(const Camera&){};
};

///////////////////////////////////////////////////////////////////////////////
// Demo app creation
struct AppCreationContext
{
  VkDevice device;
  VkPhysicalDevice physicalDevice;
  VkRenderPass renderPass;
  VkExtent2D swapchainExtent;
};

using AppCreationFunc = IApp* (*)(const AppCreationContext& context);

int registerApp(const char* name, AppCreationFunc creationFunc);

#define REGISTER_APP(className) static int registered = registerApp(#className, [](const AppCreationContext& ctx) -> IApp* { return new className(ctx); });
