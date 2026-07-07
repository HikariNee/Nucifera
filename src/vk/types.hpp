#pragma once
#include "../window.hpp"
#include <vulkan/vulkan.hpp>

constexpr uint64_t FRAMES_IN_FLIGHT = 3;

struct AppInfo
{
  Window* window;
  std::string app_name;
  std::span<const char* const> instance_extensions;
  std::span<const char* const> device_extensions;
};

struct VulkanCore
{
  vk::Instance instance;
  vk::PhysicalDevice device;
  vk::Queue graphics_queue;
  vk::SurfaceKHR surface;
  vk::CommandPool pool;
  uint32_t graphics_queue_index;

  static VulkanCore new (const AppInfo);
};

struct VulkanSwapchain
{
  vk::SwapchainKHR swapchain;
  std::vector<vk::Image> image;
  std::vector<vk::ImageView> image_view;
  vk::Extent2D extent;

  static VulkanSwapchain new (const VulkanCore&);
};

struct VulkanFrame
{
  std::vector<vk::CommandBuffer> buffer;
  std::vector<vk::Semaphore> present_semaphore;
  std::vector<vk::Semaphore> render_semaphore;
  std::vector<vk::Fence> draw_fence;

  static VulkanFrame new (const VulkanCore&, const VulkanSwapchain&);
};

struct Cosentinii
{
  VulkanCore state;
  VulkanSwapchain swapchain;
  VulkanFrame frame;

  static Cosentinii new (const AppInfo);
};
