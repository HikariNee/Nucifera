#pragma once
#include <vulkan/vulkan.hpp>

namespace shader
{
struct ShaderSet
{
  std::vector<vk::ShaderEXT> shader_objects;
  std::vector<vk::ShaderStageFlagBits> stages;
};

struct ShaderInfo
{
  std::vector<char> code;
  std::string entry_point;
  vk::ShaderStageFlagBits stage;
  vk::ShaderStageFlags next_stage;

  static ShaderInfo make(std::vector<char> a_code,
                         const std::string& a_entry_point,
                         vk::ShaderStageFlagBits a_stage,
                         vk::ShaderStageFlags a_next_stage)
  {
    return ShaderInfo{std::move(a_code), a_entry_point, a_stage, a_next_stage};
  }
};

auto create_shaders(const vk::Device, std::initializer_list<ShaderInfo>)
    -> ShaderSet;
} // namespace shader
