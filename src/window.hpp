#pragma once

#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>
#include <spdlog/spdlog.h>
#include <utility>
#include <vulkan/vulkan.hpp>

namespace window
{
struct Window
{
  SDL_Window* m_window{};
  bool m_minimized = false;
  bool m_running = false;

  static void init()
  {
    if (!SDL_Init(SDL_INIT_VIDEO))
    {
      SPDLOG_ERROR("SDL could not be initialized! %s", SDL_GetError());
      std::abort();
    }
  };

  bool is_minimized() { return m_minimized; };
  bool is_running() { return m_running; };
  auto poll_event(SDL_Event* event) { return SDL_PollEvent(event); };
  auto close()
  {
    m_running = false;
    SDL_DestroyWindow(m_window);
  };

  auto surface(const vk::Instance a_instance) -> vk::SurfaceKHR
  {
    VkSurfaceKHR surface;

    if (!SDL_Vulkan_CreateSurface(this->m_window, a_instance, nullptr,
                                  &surface))
    {
      SPDLOG_ERROR("Could not create a surface: {}", SDL_GetError());
      std::abort();
    }

    return static_cast<vk::SurfaceKHR>(surface);
  };

  std::pair<unsigned, unsigned> size()
  {
    int width;
    int height;

    if (!SDL_GetWindowSizeInPixels(m_window, &width, &height))
    {
      SPDLOG_ERROR("Could not get the size of the window! %s", SDL_GetError());
      std::abort();
    }

    return std::make_pair(width, height);
  };
};

inline auto create_window(const std::string& a_name, unsigned a_width,
                          unsigned a_height) -> Window
{
  Window::init();
  SDL_Window* window =
      SDL_CreateWindow(a_name.c_str(), a_width, a_height,
                       SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE);

  if (!window)
  {
    SPDLOG_ERROR("Could not create a window! %s", SDL_GetError());
  }

  return Window{window, false, true};
}

inline auto extensions() -> std::vector<const char*>
{
  uint32_t count;

  char const* const* extensions = SDL_Vulkan_GetInstanceExtensions(&count);

  if (!extensions)
  {
    SPDLOG_ERROR("Could not get instance extensions! %s", SDL_GetError());
  }

  std::vector<const char*> extensions_vec(extensions, extensions + count);

  return extensions_vec;
}
} // namespace window
