#pragma once
#include <glm/glm.hpp>
#include <vulkan/vulkan.hpp>

namespace shader
{
struct Vertex
{
  glm::vec2 position;
  glm::vec3 color;

  static vk::VertexInputBindingDescription2EXT binding_description()
  {
    return {.binding = 0,
            .stride = sizeof(Vertex),
            .inputRate = vk::VertexInputRate::eVertex,
            .divisor = 1};
  }

  static std::array<vk::VertexInputAttributeDescription2EXT, 2>
  attribute_description()
  {
    return {{{.location = 0,
              .binding = 0,
              .format = vk::Format::eR32G32Sfloat,
              .offset = offsetof(Vertex, position)},
             {.location = 1,
              .binding = 0,
              .format = vk::Format::eR32G32B32Sfloat,
              .offset = offsetof(Vertex, color)}}};
  }
};

const std::vector<Vertex> vertices = {{{0.4f, -0.5f}, {0.3f, 0.0f, 0.0f}},
                                      {{0.5f, 0.5f}, {0.0f, 0.5f, 0.0f}},
                                      {{-0.5f, 0.5f}, {0.0f, 0.0f, 0.5f}}};

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
