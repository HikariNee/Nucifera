#pragma once
#include <vulkan/vulkan.hpp>

constexpr uint64_t FRAMES_IN_FLIGHT = 3;

namespace window
{
struct Window;
}

struct AppInfo
{
  window::Window* window;
  std::string app_name;
  std::span<const char* const> instance_extensions;
  std::span<const char* const> device_extensions;
};

struct VulkanCore
{
  window::Window* window;
  vk::Instance instance;
  vk::PhysicalDevice physical_device;
  vk::Device device;
  vk::Queue graphics_queue;
  vk::SurfaceKHR surface;
  vk::CommandPool pool;
  uint32_t graphics_queue_index;

  static VulkanCore create(const AppInfo);
};

struct VulkanSwapchain
{
  vk::SwapchainKHR swapchain;
  std::vector<vk::Image> image;
  std::vector<vk::ImageView> image_view;
  vk::Extent2D extent;
  vk::Format format;

  static VulkanSwapchain create(const VulkanCore&);
  vk::Image get_image(uint32_t);
  vk::ImageView get_image_view(uint32_t);
};

struct VulkanFrame
{
  std::vector<vk::CommandBuffer> buffer;
  std::vector<vk::Semaphore> present_semaphore;
  std::vector<vk::Semaphore> render_semaphore;
  std::vector<vk::Fence> draw_fence;

  static VulkanFrame create(const VulkanCore&, const VulkanSwapchain&);
};

struct Cosentinii
{
  VulkanCore state;
  VulkanSwapchain swapchain;
  VulkanFrame frame;
  uint32_t index = 0;

  static Cosentinii create(const AppInfo);
  void draw_frame();
};
