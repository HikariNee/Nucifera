#pragma once

#include <SFML/Window.hpp>
#include <spdlog/spdlog.h>
#include <utility>
#include <vulkan/vulkan.hpp>

namespace window
{
struct Window
{
  sf::Window m_window{};
  bool m_minimized = false;
  bool m_running = false;

  bool is_minimized() { return m_minimized; };
  bool is_running() { return m_running; };
  bool is_open() { return m_window.isOpen(); };
  auto poll_event() { return m_window.pollEvent(); };
  auto close() { return m_window.close(); };

  auto surface(const vk::Instance a_instance) -> vk::SurfaceKHR
  {
    VkSurfaceKHR surface;

    if (!this->m_window.createVulkanSurface(a_instance, surface))
    {
      SPDLOG_ERROR("Could not create a surface.");
      std::abort();
    }

    return static_cast<vk::SurfaceKHR>(surface);
  };

  std::pair<unsigned, unsigned> size()
  {
    auto [width, height] = m_window.getSize();
    return std::make_pair(width, height);
  };
};

inline auto create_window(const std::string& a_name, unsigned a_width,
                          unsigned a_height) -> Window
{
  if (!sf::Vulkan::isAvailable(true))
  {
    SPDLOG_ERROR("Vulkan is not supported on this system.");
    std::abort();
  }

  sf::Window window{sf::VideoMode({a_width, a_height}), a_name};

  return Window{std::move(window), false, true};
}

inline auto extensions() -> std::vector<const char*>
{
  return sf::Vulkan::getGraphicsRequiredInstanceExtensions();
}
} // namespace window
