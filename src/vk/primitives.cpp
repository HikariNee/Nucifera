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

  const std::vector<vk::ExtensionProperties> extension_properties =
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
    const vk::Instance a_instance, std::span<const char* const> a_extensions)
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
        vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT,
        vk::PhysicalDeviceShaderObjectFeaturesEXT>();

    bool supports_required_features =
        features.template get<vk::PhysicalDeviceVulkan11Features>()
            .shaderDrawParameters &&
        features.template get<vk::PhysicalDeviceVulkan13Features>()
            .dynamicRendering &&
        features.template get<vk::PhysicalDeviceVulkan13Features>()
            .synchronization2 &&
        features
            .template get<vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT>()
            .extendedDynamicState &&
        features.template get<vk::PhysicalDeviceShaderObjectFeaturesEXT>()
            .shaderObject;

    if (!supports_required_features)
      continue;

    SPDLOG_INFO("Chose {}.", device_name);
    return device;
  }

  return {};
}

auto primitives::queue_index(const vk::PhysicalDevice a_physical_device,
                             const vk::QueueFlagBits flag,
                             std::function<bool(uint32_t)> func) -> uint32_t
{
  auto queue_family_properties = a_physical_device.getQueueFamilyProperties();

  uint32_t queue_position = 0;
  bool found = false;
  for (const auto& [index, qfp] :
       std::views::enumerate(queue_family_properties))
  {
    bool supportsFlag =
        (qfp.queueFlags & flag) != static_cast<vk::QueueFlags>(0);
    bool supportsFunc = func(index);

    if (supportsFlag && supportsFunc)
    {
      queue_position = index;
      found = true;
    }
  }

  if (!found)
    std::abort();

  return queue_position;
}

auto primitives::create_device(const vk::PhysicalDevice a_physical_device,
                               const vk::SurfaceKHR a_surface,
                               std::span<const char* const> a_extensions)
    -> vk::Device
{
  auto graphics_queue_index = queue_index(
      a_physical_device, vk::QueueFlagBits::eGraphics,
      [&a_physical_device, &a_surface](uint32_t queue)
      {
        return a_physical_device.getSurfaceSupportKHR(queue, a_surface).value;
      });

  float queue_priority = 0.5f;

  vk::DeviceQueueCreateInfo device_queue_create_info{
      .queueFamilyIndex = graphics_queue_index,
      .queueCount = 1,
      .pQueuePriorities = &queue_priority,
  };

  vk::StructureChain<vk::PhysicalDeviceFeatures2,
                     vk::PhysicalDeviceVulkan11Features,
                     vk::PhysicalDeviceVulkan13Features,
                     vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT,
                     vk::PhysicalDeviceShaderObjectFeaturesEXT>
      feature_chain = {
          {},
          {.shaderDrawParameters = true},
          {.synchronization2 = true, .dynamicRendering = true},
          {.extendedDynamicState = true},
          {.shaderObject = true},
      };

  vk::DeviceCreateInfo device_create_info{
      .pNext = &feature_chain.get<vk::PhysicalDeviceFeatures2>(),
      .queueCreateInfoCount = 1,
      .pQueueCreateInfos = &device_queue_create_info,
      .enabledExtensionCount = static_cast<uint32_t>(a_extensions.size()),
      .ppEnabledExtensionNames = a_extensions.data(),
  };

  return EXPECT_ABORT(a_physical_device.createDevice(device_create_info));
}

auto primitives::create_queue(const vk::Device a_device, uint32_t a_index)
    -> vk::Queue
{
  return a_device.getQueue(a_index, 0);
}

auto primitives::create_command_pool(const vk::Device a_device,
                                     uint32_t a_index) -> vk::CommandPool
{
  vk::CommandPoolCreateInfo pool_info{
      .flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer,
      .queueFamilyIndex = a_index};

  return EXPECT_ABORT(a_device.createCommandPool(pool_info));
}

auto primitives::create_command_buffers(const vk::Device a_device,
                                        const vk::CommandPool a_pool,
                                        uint32_t number)
    -> std::vector<vk::CommandBuffer>
{
  vk::CommandBufferAllocateInfo allocate_info{
      .commandPool = a_pool,
      .level = vk::CommandBufferLevel::ePrimary,
      .commandBufferCount = number,
  };

  return EXPECT_ABORT(a_device.allocateCommandBuffers(allocate_info));
}

auto primitives::get_capabilities(const vk::PhysicalDevice a_physical_device,
                                  const vk::SurfaceKHR a_surface,
                                  std::pair<uint32_t, uint32_t> a_size)
    -> std::optional<Capabilities>
{
  auto capabilities =
      EXPECT_OPTIONAL(a_physical_device.getSurfaceCapabilitiesKHR(a_surface));

  uint32_t min_image_count = std::max(3u, capabilities.minImageCount);

  if (0 < min_image_count && capabilities.maxImageCount < min_image_count)
  {
    min_image_count = capabilities.maxImageCount;
  }

  if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max())
  {
    return Capabilities{
        capabilities.currentExtent,
        min_image_count,
        capabilities.currentTransform,
    };
  }

  auto width = std::get<0>(a_size);
  auto height = std::get<1>(a_size);

  auto extent_width =
      std::clamp<uint32_t>(width, capabilities.minImageExtent.width,
                           capabilities.maxImageExtent.width);
  auto extent_height =
      std::clamp<uint32_t>(height, capabilities.minImageExtent.height,
                           capabilities.maxImageExtent.height);

  return Capabilities{
      vk::Extent2D{extent_width, extent_height},
      min_image_count,
      capabilities.currentTransform,
  };
}

auto primitives::get_surface_format(const vk::PhysicalDevice a_physical_device,
                                    const vk::SurfaceKHR a_surface)
    -> std::optional<vk::SurfaceFormatKHR>
{
  auto formats =
      EXPECT_OPTIONAL(a_physical_device.getSurfaceFormatsKHR(a_surface));

  const auto formatIt = std::ranges::find_if(
      formats,
      [](const auto& format)
      {
        return format.format == vk::Format::eB8G8R8A8Srgb &&
               format.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear;
      });

  return formatIt != formats.end() ? *formatIt : formats[0];
}

auto primitives::create_swapchain(const Capabilities& a_capabilities,
                                  const vk::SurfaceFormatKHR a_format,
                                  const vk::Device a_device,
                                  const vk::SurfaceKHR a_surface)
    -> vk::SwapchainKHR
{
  vk::SwapchainCreateInfoKHR swap_chain_create_info{
      .surface = a_surface,
      .minImageCount = a_capabilities.min_image_count,
      .imageFormat = a_format.format,
      .imageColorSpace = a_format.colorSpace,
      .imageExtent = a_capabilities.extent,
      .imageArrayLayers = 1,
      .imageUsage = vk::ImageUsageFlagBits::eColorAttachment,
      .imageSharingMode = vk::SharingMode::eExclusive,
      .preTransform = a_capabilities.transform,
      .compositeAlpha = vk::CompositeAlphaFlagBitsKHR::eOpaque,
      .presentMode = vk::PresentModeKHR::eFifo,
      .clipped = true};

  auto swapchain =
      EXPECT_ABORT(a_device.createSwapchainKHR(swap_chain_create_info));

  return swapchain;
};

auto primitives::create_swapchain_images(const vk::Device a_device,
                                         const vk::SwapchainKHR a_swapchain)
    -> std::optional<std::vector<vk::Image>>
{
  return EXPECT_OPTIONAL(a_device.getSwapchainImagesKHR(a_swapchain));
}

auto primitives::create_swapchain_views(const vk::Device a_device,
                                        const std::vector<vk::Image>& a_images,
                                        const vk::Format a_format)
    -> std::optional<std::vector<vk::ImageView>>
{
  std::vector<vk::ImageView> swapchain_image_views;
  swapchain_image_views.reserve(a_images.size());

  vk::ImageViewCreateInfo image_view_create_info{
      .viewType = vk::ImageViewType::e2D,
      .format = a_format,
      .subresourceRange = {vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1}};

  for (auto& image : a_images)
  {
    image_view_create_info.image = image;
    auto image_view =
        EXPECT_OPTIONAL(a_device.createImageView(image_view_create_info));

    swapchain_image_views.push_back(image_view);
  }

  return swapchain_image_views;
}
