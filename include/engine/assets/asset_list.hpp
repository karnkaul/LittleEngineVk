#pragma once
#include <core/io/path.hpp>
#include <core/utils/std_hash.hpp>
#include <dumb_tasks/scheduler.hpp>
#include <engine/assets/asset_store.hpp>

namespace le {
template <typename T>
class AssetList {
  public:
	using DataMap = std::unordered_map<io::Path, AssetLoadData<T>>;
	using Load = std::vector<dts::scheduler::task_t>;

	template <typename... U>
	AssetLoadData<T>& add(io::Path const& id, U&&... u);
	bool contains(io::Path const& id) const noexcept;

	Load callbacks(AssetStore& store) const;
	std::size_t append(AssetList<T>& out, AssetList<T> const& exclude) const;

	DataMap m_data;
};

template <typename T>
AssetList<T> operator-(AssetList<T> const& lhs, AssetList<T> const& rhs);
template <typename T>
AssetList<T> operator+(AssetList<T> const& lhs, AssetList<T> const& rhs);

class AssetListLoader {
  public:
	using Scheduler = dts::scheduler;
	using StageID = dts::scheduler::stage_id;

	template <typename T>
	StageID stage(AssetStore& store, AssetList<T> const& list, Scheduler& scheduler, Span<StageID const> deps = {});
	template <typename T>
	void load(AssetStore& store, AssetList<T> const& list);
	bool ready(Scheduler const* scheduler) const noexcept;

	std::vector<StageID> m_staged;
};

// impl

template <typename T>
AssetList<T> operator-(AssetList<T> const& lhs, AssetList<T> const& rhs) {
	AssetList<T> ret;
	lhs.append(ret, rhs);
	return ret;
}

template <typename T>
AssetList<T> operator+(AssetList<T> const& lhs, AssetList<T> const& rhs) {
	AssetList<T> ret = rhs;
	lhs.append(ret, rhs);
	return ret;
}

template <typename T>
template <typename... U>
AssetLoadData<T>& AssetList<T>::add(io::Path const& id, U&&... u) {
	auto [it, _] = m_data.emplace(id, std::forward<U>(u)...);
	return it->second;
}

template <typename T>
bool AssetList<T>::contains(io::Path const& id) const noexcept {
	return utils::contains(m_data, id);
}

template <typename T>
typename AssetList<T>::Load AssetList<T>::callbacks(AssetStore& store) const {
	Load ret;
	for (auto& kvp : m_data) {
		if (!store.contains<T>(kvp.first)) {
			ret.push_back([&store, kvp]() mutable { store.load<T>(kvp.first, std::move(kvp.second)); });
		}
	}
	return ret;
}

template <typename T>
std::size_t AssetList<T>::append(AssetList<T>& out, AssetList<T> const& exclude) const {
	std::size_t ret = 0;
	for (auto const& kvp : m_data) {
		if (!exclude.contains(kvp.first)) {
			out.m_data.insert(kvp);
			++ret;
		}
	}
	return ret;
}

template <typename T>
AssetListLoader::StageID AssetListLoader::stage(AssetStore& store, AssetList<T> const& list, Scheduler& scheduler, Span<StageID const> deps) {
	dts::scheduler::stage_t st;
	st.tasks = list.callbacks(store);
	if (!st.tasks.empty()) {
		st.deps = {deps.begin(), deps.end()};
		auto const ret = scheduler.stage(std::move(st));
		m_staged.push_back(ret);
		return ret;
	}
	return {};
}

template <typename T>
void AssetListLoader::load(AssetStore& store, AssetList<T> const& list) {
	for (auto const& cb : list.callbacks(store)) { cb(); }
}

inline bool AssetListLoader::ready(Scheduler const* scheduler) const noexcept {
	if (!m_staged.empty()) {
		ENSURE(scheduler, "Scheduler required to check staged tasks");
		return scheduler->stages_done(m_staged);
	}
	return true;
}
} // namespace le
