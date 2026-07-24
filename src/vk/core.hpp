#pragma once
#include "shader.hpp"
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
  vk::Queue transfer_queue;
  vk::SurfaceKHR surface;
  vk::CommandPool graphics_pool;
  vk::CommandPool transfer_pool;
  uint32_t graphics_queue_index;
  uint32_t transfer_queue_index;

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

struct Buffer
{
  vk::Buffer buffer;
  vk::DeviceMemory memory;
};

struct Cosentinii
{
  VulkanCore state;
  VulkanSwapchain swapchain;
  VulkanFrame frame;
  Buffer vertex_buffer{};
  Buffer vertex_staging_buffer{};
  Buffer index_staging_buffer{};
  Buffer index_buffer{};
  uint32_t index = 0;

  static Cosentinii create(const AppInfo);
  shader::ShaderSet create_shaders(std::initializer_list<shader::ShaderInfo>);
  bool recreate_swapchain();
  void draw_frame(shader::ShaderSet);
  void create_vertex_buffer(std::span<const shader::Vertex>);
  void create_staging_buffer(uint32_t);
  void create_index_buffer(std::span<const uint16_t>);

private:
  std::optional<uint32_t>
  find_memory_type(uint32_t a_type_filter,
                   vk::MemoryPropertyFlags a_properties);

  std::pair<vk::Buffer, vk::DeviceMemory>
  create_buffer(vk::DeviceSize a_size, vk::BufferUsageFlags a_usage,
                vk::MemoryPropertyFlags a_properties);

  void copy_buffer(vk::Buffer a_src_buffer, vk::Buffer a_dst_buffer,
                   vk::DeviceSize a_size);
};
