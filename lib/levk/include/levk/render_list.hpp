#pragma once
#include <levk/render_object.hpp>
#include <vector>

namespace levk {
/// \brief Triaged list of RenderObjects.
struct RenderList {
	std::vector<RenderObject> opaque{};
	std::vector<RenderObject> transparent{};
};
} // namespace levk
