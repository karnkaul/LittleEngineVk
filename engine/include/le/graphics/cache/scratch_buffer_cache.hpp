#pragma once
#include <le/core/mono_instance.hpp>
#include <le/graphics/buffering.hpp>
#include <le/graphics/resource.hpp>
#include <vulkan/vulkan_hash.hpp>
#include <memory>
#include <unordered_map>
#include <vector>

namespace le::graphics {
class ScratchBufferCache : public MonoInstance<ScratchBufferCache> {
  public:
	auto allocate_host(vk::BufferUsageFlags usage) -> HostBuffer&;
	auto get_empty_buffer(vk::BufferUsageFlags usage) -> DeviceBuffer const&;

	auto next_frame() -> void;
	auto clear() -> void;

  private:
	struct Pool {
		std::vector<std::unique_ptr<HostBuffer>> buffers{};
		std::unique_ptr<DeviceBuffer> empty_buffer{};
		std::size_t next{};
	};

	using Map = std::unordered_map<vk::BufferUsageFlags, Pool>;

	Buffered<Map> m_maps{};
};
} // namespace le::graphics
