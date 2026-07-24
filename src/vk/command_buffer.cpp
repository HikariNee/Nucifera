#include "command_buffer.hpp"
#include "core.hpp"
#include "shader.hpp"
#include <vulkan/vulkan.hpp>

using namespace command_buffer;

auto default_shader_state(const vk::CommandBuffer a_buffer) -> void
{
  a_buffer.setPrimitiveTopologyEXT(vk::PrimitiveTopology::eTriangleList);
  a_buffer.setPrimitiveRestartEnable(VK_FALSE);
  a_buffer.setRasterizerDiscardEnable(VK_FALSE);
  a_buffer.setPolygonModeEXT(vk::PolygonMode::eFill);
  a_buffer.setRasterizationSamplesEXT(vk::SampleCountFlagBits::e1);
  a_buffer.setCullMode(vk::CullModeFlagBits::eNone);
  a_buffer.setFrontFace(vk::FrontFace::eCounterClockwise);

  // Vertex buffer stuff.
  a_buffer.setVertexInputEXT({shader::Vertex::binding_description()},
                             shader::Vertex::attribute_description());

  a_buffer.setDepthTestEnable(VK_FALSE);
  a_buffer.setDepthWriteEnable(VK_FALSE);
  a_buffer.setDepthCompareOp(vk::CompareOp::eLess);
  a_buffer.setDepthBiasEnable(VK_FALSE);
  a_buffer.setDepthBoundsTestEnable(VK_FALSE);
  a_buffer.setStencilTestEnable(VK_FALSE);
  vk::Bool32 blendEnable = VK_FALSE;
  vk::ColorComponentFlags writeMask =
      vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG |
      vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA;

  a_buffer.setColorBlendEnableEXT(0, {blendEnable});
  a_buffer.setColorWriteMaskEXT(0, {writeMask});

  vk::SampleMask sampleMask = 0xFFFFFFFF;
  a_buffer.setSampleMaskEXT(vk::SampleCountFlagBits::e1, sampleMask);
  a_buffer.setAlphaToCoverageEnableEXT(VK_FALSE);
}

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
                                 const uint32_t index,
                                 const shader::ShaderSet a_shaders) -> void
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

  // START RENDERING
  a_buffer.beginRendering(rendering_info);

  default_shader_state(a_buffer);
  a_buffer.bindShadersEXT(a_shaders.stages, a_shaders.shader_objects);

  a_buffer.setViewportWithCount(vk::Viewport(
      0.0f, 0.0f, static_cast<float>(a_state.swapchain.extent.width),
      static_cast<float>(a_state.swapchain.extent.height), 0.0f, 1.0f));

  a_buffer.setScissorWithCount(
      vk::Rect2D(vk::Offset2D(0, 0), a_state.swapchain.extent));

  a_buffer.bindVertexBuffers(0, a_state.vertex_buffer.buffer, {0});
  a_buffer.bindIndexBuffer(a_state.index_buffer.buffer, 0,
                           vk::IndexType::eUint16);
  a_buffer.drawIndexed(static_cast<uint32_t>(shader::indices.size()), 1, 0, 0,
                       0);

  a_buffer.endRendering();
  // END RENDERING

  transition_image_layout_present(a_buffer, image,
                                  vk::ImageLayout::eColorAttachmentOptimal);

  [[maybe_unused]] const auto end_result = a_buffer.end();
}
