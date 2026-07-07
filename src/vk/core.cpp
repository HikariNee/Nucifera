// clang-format off
#include "../window.hpp"
// clang-format on

#include "core.hpp"
#include "../expect.hpp"
#include "constants.hpp"
#include "primitives.hpp"
#include <spdlog/spdlog.h>
#include <vulkan/vulkan.hpp>

auto VulkanCore::create(const AppInfo a_info) -> VulkanCore
{
  auto instance =
      primitives::create_instance(a_info.app_name, a_info.instance_extensions);
  auto physical_device =
      primitives::create_physical_device(instance, a_info.device_extensions);

  auto surface = a_info.window->surface(instance);
  auto device = primitives::create_device(physical_device, surface,
                                          a_info.device_extensions);

  auto graphics_queue_index =
      primitives::queue_index(physical_device, vk::QueueFlagBits::eGraphics,
                              [](uint32_t) { return true; });

  auto graphics_queue = primitives::create_queue(device, graphics_queue_index);

  auto pool = primitives::create_command_pool(device, graphics_queue_index);

  return VulkanCore{
      a_info.window, instance, physical_device,     device, graphics_queue,
      surface,       pool,     graphics_queue_index};
}

auto VulkanSwapchain::create(const VulkanCore& a_state) -> VulkanSwapchain
{
  auto capabilities = *primitives::get_capabilities(
      a_state.physical_device, a_state.surface, a_state.window->size());

  auto format =
      *primitives::get_surface_format(a_state.physical_device, a_state.surface);

  auto swapchain = primitives::create_swapchain(
      capabilities, format, a_state.device, a_state.surface);

  auto images = *primitives::create_swapchain_images(a_state.device, swapchain);

  auto image_views = *primitives::create_swapchain_views(a_state.device, images,
                                                         format.format);

  return VulkanSwapchain{
      swapchain, images, image_views,
      vk::Extent2D{capabilities.extent.width, capabilities.extent.height},
      format.format};
}

auto VulkanFrame::create(const VulkanCore& a_state,
                         const VulkanSwapchain& a_swapchain) -> VulkanFrame
{
  auto buffers = primitives::create_command_buffers(
      a_state.device, a_state.pool, FRAMES_IN_FLIGHT);

  std::vector<vk::Semaphore> render_semaphores;
  render_semaphores.reserve(a_swapchain.image.size());

  std::vector<vk::Semaphore> present_semaphores;
  present_semaphores.reserve(FRAMES_IN_FLIGHT);

  std::vector<vk::Fence> draw_fences;
  draw_fences.reserve(FRAMES_IN_FLIGHT);

  for (size_t i = 0; i < a_swapchain.image.size(); i++)
  {
    auto semaphore =
        EXPECT_ABORT(a_state.device.createSemaphore(vk::SemaphoreCreateInfo()));
    render_semaphores.push_back(semaphore);
  }

  for (size_t i = 0; i < FRAMES_IN_FLIGHT; i++)
  {
    auto semaphore =
        EXPECT_ABORT(a_state.device.createSemaphore(vk::SemaphoreCreateInfo()));

    auto fence = EXPECT_ABORT(a_state.device.createFence(
        vk::FenceCreateInfo{.flags = vk::FenceCreateFlagBits::eSignaled}));
    present_semaphores.push_back(semaphore);

    draw_fences.push_back(fence);
  }

  return VulkanFrame{buffers, present_semaphores, render_semaphores,
                     draw_fences};
}

auto Cosentinii::create(const AppInfo a_info) -> Cosentinii
{
  auto state = VulkanCore::create(a_info);
  auto swapchain = VulkanSwapchain::create(state);
  auto frame_data = VulkanFrame::create(state, swapchain);

  return Cosentinii{state, swapchain, frame_data};
}
