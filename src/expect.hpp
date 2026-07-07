#include <expected>
#include <vulkan/vulkan.hpp>

#define EXPECT_VK(expr)                                                        \
  ({                                                                           \
    auto&& ref = (expr);                                                       \
    if (ref != vk::Result::eSuccess)                                           \
    {                                                                          \
      return std::unexpected(ref.result);                                      \
    }                                                                          \
    *std::forward<decltype(ref)>(ref);                                         \
  })

#define EXPECT_ABORT(expr)                                                     \
  ({                                                                           \
    auto&& ref = (expr);                                                       \
    if (!ref.has_value())                                                      \
    {                                                                          \
      spdlog::error("Call failed: {}", #expr);                                 \
      std::abort();                                                            \
    }                                                                          \
    *std::forward<decltype(ref)>(ref);                                         \
  })

#define EXPECT_OPTIONAL(expr)                                                  \
  ({                                                                           \
    auto&& ref = (expr);                                                       \
    if (!ref.has_value())                                                      \
    {                                                                          \
      return std::nullopt;                                                     \
    }                                                                          \
    *std::forward<decltype(ref)>(ref);                                         \
  })
