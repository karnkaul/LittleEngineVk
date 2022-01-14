#pragma once
#include <core/span.hpp>
#include <dens/entity.hpp>
#include <unordered_set>

namespace le::edi {
class SceneRef;

class SceneTree {
  public:
	static constexpr std::string_view title_v = "Scene";

	static void attach(Span<dens::entity const> entities);
	static void detach(Span<dens::entity const> entities);

  private:
	static void update(SceneRef scene);
	static void clear();

	inline static std::unordered_set<dens::entity, dens::entity::hasher> s_custom;
	friend class Sudo;
};
} // namespace le::edi
