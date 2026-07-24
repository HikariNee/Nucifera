#define SDL_MAIN_USE_CALLBACKS
#include "expect.hpp"
#include "vk/vk.hpp"
#include "window.hpp"
#include <SDL3/SDL_main.h>
#include <chrono>
#include <iostream>
#include <thread>

struct AppState
{
  window::Window window;
  Cosentinii engine;
  shader::ShaderSet shader;
  bool need_swapchain_recreation;
};

AppState app_state;

extern "C" auto SDL_AppInit(void** a_app_state, int, char**) -> SDL_AppResult
{

  app_state.window = window::create_window("Vulkan :3", 600, 400);

  auto extensions = window::extensions();
  extensions.push_back("VK_EXT_debug_utils");

  constexpr std::array device_extensions = {
      "VK_EXT_shader_object", "VK_KHR_swapchain", "VK_KHR_synchronization2"};

  auto state =
      AppInfo{&app_state.window, "Vulkan :3", extensions, device_extensions};
  auto engine = Cosentinii::create(std::move(state));

  auto size = sizeof(shader::vertices[0]) * shader::vertices.size();
  engine.create_staging_buffer(size);
  engine.create_index_buffer(shader::indices);
  engine.create_vertex_buffer(shader::vertices);

  auto triangle_shader_src = primitives::read_shader("shaders/slang.spv");

  auto triangle_vertex_shader = shader::ShaderInfo::make(
      triangle_shader_src, "vert_main", vk::ShaderStageFlagBits::eVertex,
      vk::ShaderStageFlagBits::eFragment);

  auto triangle_fragment_shader = shader::ShaderInfo::make(
      triangle_shader_src, "frag_main", vk::ShaderStageFlagBits::eFragment, {});

  shader::ShaderSet triangle_shader =
      engine.create_shaders({triangle_vertex_shader, triangle_fragment_shader});

  app_state.engine = std::move(engine);
  app_state.shader = std::move(triangle_shader);
  app_state.need_swapchain_recreation = false;
  *a_app_state = &app_state;

  // SPDLOG_INFO("Created engine.");
  return SDL_APP_CONTINUE;
}

extern "C" auto SDL_AppEvent(void* a_app_state, SDL_Event* event)
    -> SDL_AppResult
{
  AppState* state = reinterpret_cast<AppState*>(a_app_state);

  switch (event->type)
  {
  case SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED:
    state->need_swapchain_recreation = true;
    break;

  case SDL_EVENT_QUIT:
    state->window.close();
    return SDL_APP_SUCCESS;
  }

  return SDL_APP_CONTINUE;
}

extern "C" auto SDL_AppIterate(void* a_app_state) -> SDL_AppResult
{
  AppState* state = reinterpret_cast<AppState*>(a_app_state);

  if (state->need_swapchain_recreation)
  {
    bool status = state->engine.recreate_swapchain();

    // If status is false (window is minimized) then set to true, else false.
    state->window.m_minimized = !status;
    state->need_swapchain_recreation = !status;
  }

  if (state->window.m_minimized)
  {
    std::this_thread::sleep_for(std::chrono::milliseconds(16));
    return SDL_APP_CONTINUE;
  }

  state->engine.draw_frame(state->shader);
  return SDL_APP_CONTINUE;
}

extern "C" auto SDL_AppQuit(void*, SDL_AppResult) -> void {}
