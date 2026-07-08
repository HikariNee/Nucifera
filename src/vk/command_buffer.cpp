#include "command_buffer.hpp"
#include "core.hpp"
#include <vulkan/vulkan.hpp>

using namespace command_buffer;

auto command_buffer::transition_image_layout(const vk::CommandBuffer a_buffer,
                                             const TransitionInfo& a_info)
    -> void
{
  vk::ImageMemoryBarrier2 barrier = {
      .srcStageMask = a_info.src_stage_mask,
      .srcAccessMask = a_info.src_access_mask,
      .dstStageMask = a_info.dst_stage_mask,
      .dstAccessMask = a_info.dst_access_mask,
      .oldLayout = a_info.old_layout,
      .newLayout = a_info.new_layout,
      .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
      .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
      .image = a_info.image,
      .subresourceRange = {.aspectMask = vk::ImageAspectFlagBits::eColor,
                           .baseMipLevel = 0,
                           .levelCount = 1,
                           .baseArrayLayer = 0,
                           .layerCount = 1}};

  vk::DependencyInfo dependency_info = {.dependencyFlags = {},
                                        .imageMemoryBarrierCount = 1,
                                        .pImageMemoryBarriers = &barrier};

  a_buffer.pipelineBarrier2(dependency_info);
}

auto command_buffer::transition_image_layout_optimal(
    const vk::CommandBuffer a_buffer, const vk::Image a_image,
    const vk::ImageLayout a_old_layout) -> void
{
  const TransitionInfo transition_info = {
      .image = a_image,
      .old_layout = a_old_layout,
      .new_layout = vk::ImageLayout::eColorAttachmentOptimal,
      .src_stage_mask = vk::PipelineStageFlagBits2::eColorAttachmentOutput,
      .src_access_mask = {},
      .dst_stage_mask = vk::PipelineStageFlagBits2::eColorAttachmentOutput,
      .dst_access_mask = vk::AccessFlagBits2::eColorAttachmentWrite};

  command_buffer::transition_image_layout(a_buffer, transition_info);
}

auto command_buffer::transition_image_layout_present(
    const vk::CommandBuffer a_buffer, const vk::Image a_image,
    const vk::ImageLayout a_old_layout) -> void
{
  const TransitionInfo transition_info = {
      .image = a_image,
      .old_layout = a_old_layout,
      .new_layout = vk::ImageLayout::ePresentSrcKHR,
      .src_stage_mask = vk::PipelineStageFlagBits2::eColorAttachmentOutput,
      .src_access_mask = vk::AccessFlagBits2::eColorAttachmentWrite,
      .dst_stage_mask = vk::PipelineStageFlagBits2::eBottomOfPipe,
      .dst_access_mask = {},
  };

  command_buffer::transition_image_layout(a_buffer, transition_info);
}

auto command_buffer::clear_image(Cosentinii& a_state,
                                 const vk::CommandBuffer a_buffer,
                                 const uint32_t index) -> void
{
  vk::ClearValue clear_colour = {
      .color = vk::ClearColorValue{{{0.5f, 0.5f, 0.5f, 1.0f}}}};

  const auto image_view = a_state.swapchain.get_image_view(index);
  const auto image = a_state.swapchain.get_image(index);

  vk::CommandBufferBeginInfo begin_info{};
  const auto _ = a_buffer.begin(begin_info);

  transition_image_layout_optimal(a_buffer, image, vk::ImageLayout::eUndefined);

  const vk::RenderingAttachmentInfo rendering_attachment_info = {
      .imageView = image_view,
      .imageLayout = vk::ImageLayout::eColorAttachmentOptimal,
      .loadOp = vk::AttachmentLoadOp::eClear,
      .storeOp = vk::AttachmentStoreOp::eStore,
      .clearValue = clear_colour,
  };

  const vk::RenderingInfo rendering_info = {
      .renderArea = {.offset = {0, 0}, .extent = a_state.swapchain.extent},
      .layerCount = 1,
      .colorAttachmentCount = 1,
      .pColorAttachments = &rendering_attachment_info};

  a_buffer.beginRendering(rendering_info);
  a_buffer.endRendering();

  transition_image_layout_present(a_buffer, image,
                                  vk::ImageLayout::eColorAttachmentOptimal);

  [[maybe_unused]] const auto end_result = a_buffer.end();
}
