#pragma once
#include <functional>
#include <core/io/path.hpp>
#include <core/utils/std_hash.hpp>
#include <dumb_tasks/scheduler.hpp>
#include <engine/assets/asset_store.hpp>

namespace le {
template <typename T>
struct TAssetList {
	void add(io::Path const& id, T t) { map.emplace(id, std::move(t)); }
	std::size_t append(TAssetList<T>& out, TAssetList<T> const& exclude) const;

	std::unordered_map<io::Path, T> map;
};

template <typename T>
using AssetList = TAssetList<std::function<T()>>;

template <typename T>
using AssetLoadList = TAssetList<AssetLoadData<T>>;

template <typename T>
AssetLoadList<T> operator-(AssetLoadList<T> const& lhs, AssetLoadList<T> const& rhs);
template <typename T>
AssetLoadList<T> operator+(AssetLoadList<T> const& lhs, AssetLoadList<T> const& rhs);

class AssetListLoader {
  public:
	enum class Mode { eAsync, eImmediate };

	using Scheduler = dts::scheduler;
	using StageID = dts::scheduler::stage_id;

	template <typename T>
	StageID stage(AssetStore& store, TAssetList<T> list, Scheduler& scheduler, Span<StageID const> deps = {});
	template <typename T>
	void load(AssetStore& store, TAssetList<T> list);
	bool ready(Scheduler const& scheduler) const noexcept;

	Mode m_mode = Mode::eAsync;

  private:
	template <typename T>
	std::vector<dts::scheduler::task_t> callbacks(AssetStore& store, AssetLoadList<T> list) const;
	template <typename T>
	std::vector<dts::scheduler::task_t> callbacks(AssetStore& store, AssetList<T> list) const;
	template <typename T>
	void loadImpl(AssetStore& store, AssetLoadList<T> list) const;
	template <typename T>
	void loadImpl(AssetStore& store, AssetList<T> list) const;

	std::vector<StageID> m_staged;
};

// impl

template <typename T>
AssetLoadList<T> operator-(AssetLoadList<T> const& lhs, AssetLoadList<T> const& rhs) {
	AssetLoadList<T> ret;
	lhs.append(ret, rhs);
	return ret;
}

template <typename T>
AssetLoadList<T> operator+(AssetLoadList<T> const& lhs, AssetLoadList<T> const& rhs) {
	AssetLoadList<T> ret = rhs;
	lhs.append(ret, rhs);
	return ret;
}

template <typename T>
std::size_t TAssetList<T>::append(TAssetList<T>& out, TAssetList<T> const& exclude) const {
	std::size_t ret = 0;
	for (auto const& kvp : map) {
		if (!exclude.contains(kvp.first)) {
			out.map.insert(kvp);
			++ret;
		}
	}
	return ret;
}

template <typename T>
AssetListLoader::StageID AssetListLoader::stage(AssetStore& store, TAssetList<T> list, Scheduler& scheduler, Span<StageID const> deps) {
	if (m_mode == Mode::eImmediate) {
		loadImpl(store, std::move(list));
	} else {
		dts::scheduler::stage_t st;
		st.tasks = callbacks(store, std::move(list));
		if (!st.tasks.empty()) {
			st.deps = {deps.begin(), deps.end()};
			auto const ret = scheduler.stage(std::move(st));
			m_staged.push_back(ret);
			return ret;
		}
	}
	return {};
}

template <typename T>
void AssetListLoader::load(AssetStore& store, TAssetList<T> list) {
	loadImpl(store, std::move(list));
}

inline bool AssetListLoader::ready(Scheduler const& scheduler) const noexcept {
	if (!m_staged.empty()) { return scheduler.stages_done(m_staged); }
	return true;
}

template <typename T>
std::vector<dts::scheduler::task_t> AssetListLoader::callbacks(AssetStore& store, AssetLoadList<T> list) const {
	std::vector<dts::scheduler::task_t> ret;
	for (auto& kvp : list.map) {
		if (!store.exists<T>(kvp.first)) {
			ret.push_back([&store, id = kvp.first, data = std::move(kvp.second)]() mutable { store.load<T>(id, std::move(data)); });
		}
	}
	return ret;
}

template <typename T>
std::vector<dts::scheduler::task_t> AssetListLoader::callbacks(AssetStore& store, AssetList<T> list) const {
	std::vector<dts::scheduler::task_t> ret;
	for (auto const& kvp : list.map) {
		if (!store.exists<T>(kvp.first)) {
			ret.push_back([&store, id = kvp.first, t = kvp.second]() mutable { store.add(id, t()); });
		}
	}
	return ret;
}

template <typename T>
void AssetListLoader::loadImpl(AssetStore& store, AssetLoadList<T> list) const {
	for (auto& [id, data] : list.map) { store.load<T>(id, std::move(data)); }
}

template <typename T>
void AssetListLoader::loadImpl(AssetStore& store, AssetList<T> list) const {
	for (auto& [id, cb] : list.map) { store.add(id, cb()); }
}
} // namespace le
