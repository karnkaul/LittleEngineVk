#include <engine/scene/material_map.hpp>

namespace le {
IMaterial const* MaterialMap::find(Hash uri) const noexcept {
	if (auto it = m_map.find(uri); it != m_map.end()) { return it->second.material.get(); }
	return {};
}
} // namespace le
