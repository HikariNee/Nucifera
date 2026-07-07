#include "expect.hpp"
#include "vk/vk.hpp"
#include "window.hpp"
#include <chrono>
#include <iostream>
#include <thread>

auto main() -> int
{
  using namespace std::chrono_literals;

  window::Window window = window::create_window("Vulkan :3", 600, 400);

  auto extensions = window::extensions();
  extensions.push_back("VK_EXT_debug_utils");

  constexpr std::array device_extensions{"VK_EXT_shader_object",
                                         "VK_KHR_swapchain"};

  auto state = AppInfo{&window, "Vulkan :3", extensions, device_extensions};

  auto engine = Cosentinii::create(state);

  while (window.is_open())
  {
    while (const std::optional event = window.poll_event())
    {
      if (event->is<sf::Event::Closed>())
        window.close();
    }

    std::this_thread::sleep_for(16ms);
  }

  return 0;
}
