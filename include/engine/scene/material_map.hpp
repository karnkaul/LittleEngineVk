#pragma once
#include <engine/scene/material.hpp>
#include <graphics/render/pipeline_spec.hpp>
#include <memory>
#include <unordered_map>

namespace le {
class MaterialMap {
  public:
	using State = graphics::PipelineSpec;

	template <typename T, typename... Args>
	T& attach(Hash uri, State state, Args&&... args) {
		return *m_map.insert_or_assign(uri, Entry{state, std::make_unique<T>(std::forward<Args>(args)...)}).first->second.material;
	}

	IMaterial const* find(Hash uri) const noexcept;
	bool contains(Hash uri) const noexcept { return find(uri) != nullptr; }
	void erase(Hash uri) { m_map.erase(uri); }
	void clear() noexcept { m_map.clear(); }
	std::size_t size() const noexcept { return m_map.size(); }
	bool empty() const noexcept { return m_map.empty(); }

  private:
	struct Entry {
		State state;
		std::unique_ptr<IMaterial> material;
	};
	std::unordered_map<Hash, Entry> m_map;
};
} // namespace le
