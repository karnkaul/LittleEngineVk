#pragma once
#include <ktl/either.hpp>
#include <levk/core/span.hpp>

namespace le {
namespace graphics {
class MeshPrimitive;
}
using MeshPrimitive = graphics::MeshPrimitive;
struct Material;

struct MeshObj {
	MeshPrimitive const* primitive = {};
	Material const* material = {};
};

using MeshObjView = Span<MeshObj const>;

class MeshView {
  public:
	MeshView(MeshObjView view = {}) noexcept : m_objects(view) {}
	MeshView(MeshObj object) noexcept : m_objects(object) {}

	MeshObjView meshViews() const noexcept;
	MeshObj front() const noexcept { return empty() ? MeshObj{} : meshViews().front(); }
	bool empty() const noexcept;

  private:
	ktl::either<MeshObjView, MeshObj> m_objects;
};

// impl

inline MeshObjView MeshView::meshViews() const noexcept {
	if (m_objects.contains<MeshObj>()) {
		return m_objects.get<MeshObj>();
	} else {
		return m_objects.get<MeshObjView>();
	}
}

inline bool MeshView::empty() const noexcept {
	return m_objects.visit(ktl::koverloaded{
		[](MeshObjView const& view) { return view.empty(); },
		[](MeshObj const&) { return false; },
	});
}
} // namespace le
