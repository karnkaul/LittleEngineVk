#pragma once
#include <core/span.hpp>
#include <dens/entity.hpp>
#include <ktl/either.hpp>

namespace dens {
class registry;
}

namespace le {
class Editor;

namespace gui {
class TreeRoot;
}

namespace edi {
using Inspect = ktl::either<dens::entity, gui::TreeRoot*>;

class SceneRef {
  public:
	SceneRef() = default;
	SceneRef(dens::registry& out_registry, dens::entity root, Span<dens::entity const> custom = {}) noexcept
		: m_custom(custom), m_registry(&out_registry), m_root(root) {}

	bool valid() const noexcept { return m_registry; }
	dens::registry const& registry() const { return *m_registry; }
	dens::entity root() const noexcept { return m_root; }
	Span<dens::entity const> custom() const noexcept { return m_custom; }

  private:
	Span<dens::entity const> m_custom;
	dens::registry* m_registry{};
	dens::entity m_root{};
	Inspect* m_inspect{};

	friend class ::le::Editor;
	friend class Inspector;
	friend class SceneTree;
};

} // namespace edi
} // namespace le
