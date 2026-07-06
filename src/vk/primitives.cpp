#include "primitives.hpp"
#include "../expect.hpp"
#include <ranges>
#include <span>
#include <spdlog/spdlog.h>
#include <vulkan/vulkan_funcs.hpp>

auto primitives::create_instance(const std::string& a_name,
                                 std::span<const char* const> a_extensions)
    -> vk::Instance
{
  const vk::ApplicationInfo app_info{
      .pApplicationName = a_name.c_str(),
      .applicationVersion = VK_MAKE_VERSION(0, 0, 1),
      .pEngineName = "Cosentinii",
      .engineVersion = VK_MAKE_VERSION(0, 0, 1),
      .apiVersion = vk::ApiVersion13,
  };

  const auto extension_properties =
      EXPECT_ABORT(vk::enumerateInstanceExtensionProperties(nullptr));

  for (const auto& extension : a_extensions)
  {
    bool supports_extension = false;

    for (const auto& supported_extension : extension_properties)
    {
      if (strcmp(supported_extension.extensionName, extension) == 0)
      {
        supports_extension = true;
        break;
      }
    }

    if (!supports_extension)
    {
      SPDLOG_ERROR("System does not support required extension {}", extension);
    }
  }

  const vk::InstanceCreateInfo create_info{
      .pApplicationInfo = &app_info,
      .enabledExtensionCount = static_cast<uint32_t>(a_extensions.size()),
      .ppEnabledExtensionNames = a_extensions.data(),
  };

  return EXPECT_ABORT(vk::createInstance(create_info));
}

auto primitives::create_physical_device(
    const vk::Instance& a_instance, std::span<const char* const> a_extensions)
    -> vk::PhysicalDevice
{
  auto devices = EXPECT_ABORT(a_instance.enumeratePhysicalDevices());

  if (devices.empty())
    std::abort();

  for (const auto& device : devices)
  {
    auto device_properties = device.getProperties();
    auto queue_families = device.getQueueFamilyProperties();
    auto available_extensions =
        EXPECT_ABORT(device.enumerateDeviceExtensionProperties());

    std::string_view device_name = std::string_view(
        device_properties.deviceName, VK_MAX_PHYSICAL_DEVICE_NAME_SIZE);

    SPDLOG_INFO("candidate: {}, Type: {}.", device_name,
                vk::to_string(device_properties.deviceType));

    bool supports_vulkan_13 = device_properties.apiVersion >= vk::ApiVersion13;

    SPDLOG_INFO("Supports Vulkan 13? {}.", supports_vulkan_13);

    if (!supports_vulkan_13)
      continue;

    bool supports_graphics = std::ranges::any_of(
        queue_families, [](auto const& qfp)
        { return !!(qfp.queueFlags & vk::QueueFlagBits::eGraphics); });

    SPDLOG_INFO("Has graphics queue? {}.", supports_graphics);

    if (!supports_graphics)
      continue;

    bool is_discrete =
        device_properties.deviceType == vk::PhysicalDeviceType::eDiscreteGpu;

    if (!is_discrete)
      continue;

    bool supports_all_extensions = false;
    for (const auto& extension : a_extensions)
    {
      bool supports_extension = false;
      for (const auto& device_extension : available_extensions)
      {
        if (strcmp(device_extension.extensionName, extension) == 0)
        {
          supports_extension = true;
          supports_all_extensions = true;
          break;
        }
      }

      if (!supports_extension)
      {
        supports_all_extensions = false;
        SPDLOG_ERROR("Candidate does not support {} extension.", extension);
      }
    }

    if (!supports_all_extensions)
      continue;

    auto features = device.template getFeatures2<
        vk::PhysicalDeviceFeatures2, vk::PhysicalDeviceVulkan11Features,
        vk::PhysicalDeviceVulkan13Features,
        vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT>();

    bool supports_required_features =
        features.template get<vk::PhysicalDeviceVulkan11Features>()
            .shaderDrawParameters &&
        features.template get<vk::PhysicalDeviceVulkan13Features>()
            .dynamicRendering &&
        features
            .template get<vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT>()
            .extendedDynamicState;

    if (!supports_required_features)
      continue;

    SPDLOG_INFO("Chose {}.", device_name);
    return device;
  }

  return {};
}
