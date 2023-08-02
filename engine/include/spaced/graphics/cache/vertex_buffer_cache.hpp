#pragma once
#include <spaced/core/mono_instance.hpp>
#include <spaced/graphics/buffering.hpp>
#include <spaced/graphics/resource.hpp>
#include <memory>
#include <vector>

namespace spaced::graphics {
class VertexBufferCache : public MonoInstance<VertexBufferCache> {
  public:
	auto allocate() -> Buffered<std::shared_ptr<HostBuffer>>;

	auto clear() -> void;

  private:
	std::vector<Buffered<std::shared_ptr<HostBuffer>>> m_buffers{};
};
} // namespace spaced::graphics
