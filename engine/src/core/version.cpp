#include <le/core/version.hpp>
#include <vulkan/vulkan.hpp>
#include <format>

namespace le {
auto Version::to_string() const -> std::string { return le::to_string(*this); }

auto Version::to_vk_version() const -> std::uint32_t { return VK_MAKE_VERSION(major, minor, patch); }
} // namespace le

auto le::to_string(Version const& version) -> std::string { return std::format("v{}.{}.{}", version.major, version.minor, version.patch); }
