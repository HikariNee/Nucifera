#include "shader.hpp"
#include "../expect.hpp"
#include <spdlog/spdlog.h>

using namespace shader;

auto shader::create_shaders(const vk::Device a_device,
                            std::initializer_list<ShaderInfo> a_shader_infos)
    -> ShaderSet
{
  std::vector<vk::ShaderEXT> shaders;
  std::vector<vk::ShaderStageFlagBits> stage;

  shaders.reserve(a_shader_infos.size());
  stage.reserve(a_shader_infos.size());

  for (const auto& shader_info : a_shader_infos)
  {
    const vk::ShaderCreateInfoEXT shader_create_info = {
        .stage = shader_info.stage,
        .nextStage = shader_info.next_stage,
        .codeType = vk::ShaderCodeTypeEXT::eSpirv,
        .codeSize = shader_info.code.size(),
        .pCode = shader_info.code.data(),
        .pName = shader_info.entry_point.c_str(),
    };

    const vk::ShaderEXT object =
        EXPECT_ABORT(a_device.createShaderEXT(shader_create_info));

    shaders.push_back(object);
    stage.push_back(shader_info.stage);
  }

  return ShaderSet{std::move(shaders), std::move(stage)};
}
