#pragma once

#include <functional>
#include <span>
#include <string>
#include <vulkan/vulkan.hpp>

struct VulkanSwapchain;

namespace primitives
{
struct Capabilities
{
  vk::Extent2D extent;
  uint32_t min_image_count;
  vk::SurfaceTransformFlagBitsKHR transform;
};

struct TransitionInfo
{
  vk::Image image;
  vk::ImageLayout old_layout;
  vk::ImageLayout new_layout;
  vk::PipelineStageFlags2 src_stage_mask;
  vk::AccessFlags2 src_access_mask;
  vk::PipelineStageFlags2 dst_stage_mask;
  vk::AccessFlags2 dst_access_mask;
};

auto create_instance(const std::string&, std::span<const char* const>)
    -> vk::Instance;

auto create_physical_device(const vk::Instance, std::span<const char* const>)
    -> vk::PhysicalDevice;

auto create_device(const vk::PhysicalDevice, std::span<const uint32_t>,
                   std::span<const char* const>) -> vk::Device;

auto create_queue(const vk::Device, uint32_t) -> vk::Queue;

auto create_command_pool(const vk::Device, uint32_t) -> vk::CommandPool;

auto create_command_buffers(const vk::Device, const vk::CommandPool, uint32_t)
    -> std::vector<vk::CommandBuffer>;

auto queue_index(const vk::PhysicalDevice, const vk::QueueFlagBits,
                 std::function<bool(uint32_t)>) -> uint32_t;

auto get_capabilities(const vk::PhysicalDevice, const vk::SurfaceKHR,
                      std::pair<uint32_t, uint32_t>)
    -> std::optional<Capabilities>;

auto get_surface_format(const vk::PhysicalDevice, const vk::SurfaceKHR)
    -> std::optional<vk::SurfaceFormatKHR>;

auto create_swapchain(const Capabilities&, const vk::SurfaceFormatKHR,
                      const vk::Device, const vk::SurfaceKHR,
                      const std::optional<vk::SwapchainKHR> = std::nullopt)
    -> vk::SwapchainKHR;

auto create_swapchain_images(const vk::Device, const vk::SwapchainKHR)
    -> std::optional<std::vector<vk::Image>>;
auto create_swapchain_views(const vk::Device, const std::vector<vk::Image>&,
                            const vk::Format)
    -> std::optional<std::vector<vk::ImageView>>;

auto read_shader(const std::string&) -> std::vector<char>;
} // namespace primitives
