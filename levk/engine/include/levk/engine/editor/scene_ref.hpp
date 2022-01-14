#pragma once
#include <dens/entity.hpp>
#include <ktl/either.hpp>
#include <levk/core/span.hpp>

namespace dens {
class registry;
}

namespace le {
namespace gui {
class TreeRoot;
}

namespace edi {
struct Inspecting {
	dens::entity entity;
	gui::TreeRoot* tree{};
};

class SceneRef {
  public:
	SceneRef() = default;
	SceneRef(dens::registry& out_registry, dens::entity root) noexcept : m_registry(&out_registry), m_root(root) {}

	bool valid() const noexcept { return m_registry; }
	dens::registry const& registry() const { return *m_registry; }
	dens::entity root() const noexcept { return m_root; }

  private:
	dens::registry* m_registry{};
	dens::entity m_root{};
	Inspecting* m_inspect{};

	friend class Sudo;
};
} // namespace edi
} // namespace le
