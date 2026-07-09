#pragma once
#include "shader.hpp"
#include <vulkan/vulkan.hpp>

struct Cosentinii;

namespace command_buffer
{
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

auto transition_image_layout(const vk::CommandBuffer, const TransitionInfo&)
    -> void;
auto transition_image_layout_optimal(const vk::CommandBuffer, const vk::Image,
                                     const vk::ImageLayout) -> void;
auto transition_image_layout_present(const vk::CommandBuffer, const vk::Image,
                                     const vk::ImageLayout) -> void;

auto clear_image(Cosentinii& a_state, const vk::CommandBuffer a_buffer,
                 const uint32_t index, shader::ShaderSet) -> void;
} // namespace command_buffer
