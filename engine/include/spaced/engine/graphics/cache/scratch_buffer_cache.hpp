#pragma once
#include <spaced/engine/core/mono_instance.hpp>
#include <spaced/engine/graphics/buffering.hpp>
#include <spaced/engine/graphics/resource.hpp>
#include <vulkan/vulkan_hash.hpp>
#include <memory>
#include <unordered_map>
#include <vector>

namespace spaced::graphics {
class ScratchBufferCache : public MonoInstance<ScratchBufferCache> {
  public:
	auto allocate(vk::BufferUsageFlags usage) -> HostBuffer&;
	auto empty_buffer(vk::BufferUsageFlags usage) -> DeviceBuffer const&;

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
} // namespace spaced::graphics
