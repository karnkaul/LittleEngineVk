#pragma once
#include <le/core/mono_instance.hpp>
#include <le/graphics/buffering.hpp>
#include <le/graphics/resource.hpp>
#include <memory>
#include <vector>

namespace le::graphics {
class VertexBufferCache : public MonoInstance<VertexBufferCache> {
  public:
	auto allocate() -> Buffered<std::shared_ptr<HostBuffer>>;

	[[nodiscard]] auto buffer_count() const -> std::size_t { return m_buffers.size(); }
	auto clear() -> void;

  private:
	std::vector<Buffered<std::shared_ptr<HostBuffer>>> m_buffers{};
};
} // namespace le::graphics
