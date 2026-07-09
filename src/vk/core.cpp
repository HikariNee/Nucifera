// clang-format off
#include "../window.hpp"
// clang-format on

#include "core.hpp"
#include "../expect.hpp"
#include "command_buffer.hpp"
#include "constants.hpp"
#include "primitives.hpp"
#include "shader.hpp"
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

auto VulkanSwapchain::get_image(uint32_t index) -> vk::Image
{
  return this->image[index];
}

auto VulkanSwapchain::get_image_view(uint32_t index) -> vk::ImageView
{
  return this->image_view[index];
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

auto Cosentinii::draw_frame(shader::ShaderSet a_shaders) -> void
{
  // Aliases for.. stuff
  const auto device = this->state.device;
  const auto index = this->index;
  const auto swapchain = this->swapchain.swapchain;
  const auto buffer = this->frame.buffer[index];
  const auto queue = this->state.graphics_queue;
  const auto fence = this->frame.draw_fence[index];
  const auto present_semaphore = this->frame.present_semaphore[index];

  // Fail is fence cannot be checked.
  const auto _ =
      EXPECT_ABORT_VK_RESULT(device.waitForFences(fence, vk::True, UINT64_MAX));

  // Get an Image
  auto [result, image_index] = device.acquireNextImageKHR(
      swapchain, UINT64_MAX, present_semaphore, nullptr);

  // Swapchain got invalidated. Recreate it and skip the frame.
  if (result == vk::Result::eErrorOutOfDateKHR)
  {
    recreate_swapchain();
    return;
  }

  if (result != vk::Result::eSuccess && result != vk::Result::eSuboptimalKHR)
  {
    assert(result == vk::Result::eTimeout || result == vk::Result::eNotReady);
    SPDLOG_ERROR("failed to acquire swap chain image!");
    std::abort();
  }

  [[maybe_unused]] const auto reset_fences_result = device.resetFences(fence);

  // I hate these [[maybe_unused]] but what can I do smh.
  [[maybe_unused]] const auto reset_result = buffer.reset();

  // Clear the image
  command_buffer::clear_image(*this, buffer, image_index, std::move(a_shaders));

  vk::PipelineStageFlags wait_destination_stage_mask(
      vk::PipelineStageFlagBits::eColorAttachmentOutput);

  // Prepare for submit
  const vk::SubmitInfo submit_info{
      .waitSemaphoreCount = 1,
      .pWaitSemaphores = &present_semaphore,
      .pWaitDstStageMask = &wait_destination_stage_mask,
      .commandBufferCount = 1,
      .pCommandBuffers = &buffer,
      .signalSemaphoreCount = 1,
      .pSignalSemaphores = &this->frame.render_semaphore[image_index]};

  // Submit
  [[maybe_unused]] const auto submit_result = queue.submit(submit_info, fence);

  // Prepare to show
  const vk::PresentInfoKHR present_info_khr{
      .waitSemaphoreCount = 1,
      .pWaitSemaphores = &this->frame.render_semaphore[image_index],
      .swapchainCount = 1,
      .pSwapchains = &swapchain,
      .pImageIndices = &image_index};

  // show
  [[maybe_unused]] const auto present_result =
      queue.presentKHR(present_info_khr);

  // Just recreate the swapchain and move on, we already have an image.
  if ((present_result == vk::Result::eSuboptimalKHR) ||
      (present_result == vk::Result::eErrorOutOfDateKHR))
  {
    recreate_swapchain();
  }

  // increment index to the next frame
  this->index = (index + 1) % FRAMES_IN_FLIGHT;
}

auto Cosentinii::create_shaders(
    std::initializer_list<shader::ShaderInfo> a_info) -> shader::ShaderSet
{
  return shader::create_shaders(this->state.device, a_info);
}

auto Cosentinii::recreate_swapchain() -> bool
{
  auto physical_device = this->state.physical_device;
  auto surface = this->state.surface;
  auto device = this->state.device;
  auto& swapchain = this->swapchain;

  auto new_size = this->state.window->size();

  const auto _ = device.waitIdle();

  // Window was minimized, we can't do shit.
  if (new_size == std::make_pair(0, 0))
  {
    return false;
  }

  // Cleanup the swapchain. We do not need to clear the images (only the views)
  // because they are cleared by Vulkan itself.
  swapchain.image_view.clear();

  // Make the new swapchain, we pass the old swapchain along as well to make the
  // transition seamless.
  auto capabilities = *primitives::get_capabilities(physical_device, surface,
                                                    this->state.window->size());

  auto format = *primitives::get_surface_format(physical_device, surface);

  auto new_swapchain = primitives::create_swapchain(
      capabilities, format, device, surface, swapchain.swapchain);

  auto images = *primitives::create_swapchain_images(device, new_swapchain);

  auto image_views =
      *primitives::create_swapchain_views(device, images, format.format);

  if (swapchain.swapchain)
  {
    device.destroySwapchainKHR(swapchain.swapchain);
  }

  swapchain.swapchain = new_swapchain;
  swapchain.image = images;
  swapchain.image_view = image_views;
  swapchain.extent =
      vk::Extent2D{capabilities.extent.width, capabilities.extent.height};
  swapchain.format = format.format;

  return true;
}
