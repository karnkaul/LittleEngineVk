#pragma once
#include <functional>
#include <core/io/path.hpp>
#include <core/services.hpp>
#include <core/utils/std_hash.hpp>
#include <dumb_tasks/scheduler.hpp>
#include <engine/assets/asset_store.hpp>
#include <kt/enum_flags/enum_flags.hpp>

namespace le {
template <typename T>
struct TAssetList {
	void add(io::Path id, T t) { map.emplace(std::move(id), std::move(t)); }
	std::size_t append(TAssetList<T>& out, TAssetList<T> const& exclude) const;

	std::unordered_map<io::Path, T> map;
};

template <typename T>
using AssetList = TAssetList<std::function<T()>>;

template <typename T>
using AssetLoadList = TAssetList<AssetLoadData<T>>;

template <typename T>
TAssetList<T> operator-(TAssetList<T> const& lhs, TAssetList<T> const& rhs);
template <typename T>
TAssetList<T> operator+(TAssetList<T> const& lhs, TAssetList<T> const& rhs);

class AssetListLoader {
  public:
	enum class Flag { eImmediate, eOverwrite };
	using Flags = kt::enum_flags<Flag, u8>;

	using Scheduler = dts::scheduler;
	using StageID = dts::scheduler::stage_id;
	using QueueID = dts::scheduler::queue_id;

	template <typename T>
	StageID stage(TAssetList<T> list, Scheduler* scheduler, Span<StageID const> deps = {}, QueueID qid = {});
	template <typename T>
	void load(TAssetList<T> list);
	bool ready(Scheduler const& scheduler) const noexcept;
	void wait(Scheduler& scheduler) const noexcept;

	Flags m_flags;

  private:
	template <typename T>
	std::vector<dts::scheduler::task_t> callbacks(AssetLoadList<T> list) const;
	template <typename T>
	std::vector<dts::scheduler::task_t> callbacks(AssetList<T> list) const;
	template <typename T>
	void load_(AssetLoadList<T> list) const;
	template <typename T>
	void load_(AssetList<T> list) const;

	AssetStore& store() const { return m_store ? *m_store : *(m_store = Services::locate<AssetStore>()); }
	template <typename T>
	bool skip(io::Path const& id) const noexcept {
		return store().exists<T>(id) && !m_flags.test(Flag::eOverwrite);
	}

	mutable AssetStore* m_store{};
	mutable std::vector<StageID> m_staged;
};

// impl

template <typename T>
TAssetList<T> operator-(TAssetList<T> const& lhs, TAssetList<T> const& rhs) {
	TAssetList<T> ret;
	lhs.append(ret, rhs);
	return ret;
}

template <typename T>
TAssetList<T> operator+(TAssetList<T> const& lhs, TAssetList<T> const& rhs) {
	TAssetList<T> ret = rhs;
	lhs.append(ret, rhs);
	return ret;
}

template <typename T>
std::size_t TAssetList<T>::append(TAssetList<T>& out, TAssetList<T> const& exclude) const {
	std::size_t ret = 0;
	for (auto const& kvp : map) {
		if (!exclude.map.contains(kvp.first)) {
			out.map.insert(kvp);
			++ret;
		}
	}
	return ret;
}

template <typename T>
AssetListLoader::StageID AssetListLoader::stage(TAssetList<T> list, Scheduler* scheduler, Span<StageID const> deps, QueueID qid) {
	if (m_flags.test(Flag::eImmediate) || !scheduler) {
		load_(std::move(list));
	} else {
		dts::scheduler::stage_t st;
		st.tasks = callbacks(std::move(list));
		if (!st.tasks.empty()) {
			st.deps = {deps.begin(), deps.end()};
			auto const ret = scheduler->stage(std::move(st), qid);
			m_staged.push_back(ret);
			return ret;
		}
	}
	return {};
}

template <typename T>
void AssetListLoader::load(TAssetList<T> list) {
	load_(std::move(list));
}

inline bool AssetListLoader::ready(Scheduler const& scheduler) const noexcept {
	if (!m_staged.empty() && scheduler.stages_done(m_staged)) { m_staged.clear(); }
	return m_staged.empty();
}

inline void AssetListLoader::wait(Scheduler& scheduler) const noexcept {
	if (!m_staged.empty()) {
		scheduler.wait_stages(m_staged);
		m_staged.clear();
	}
}

template <typename T>
std::vector<dts::scheduler::task_t> AssetListLoader::callbacks(AssetLoadList<T> list) const {
	std::vector<dts::scheduler::task_t> ret;
	for (auto& kvp : list.map) {
		if (!skip<T>(kvp.first)) {
			ret.push_back([s = &store(), kvp = std::move(kvp)]() mutable { s->load<T>(std::move(kvp.first), std::move(kvp.second)); });
		}
	}
	list.map.clear();
	return ret;
}

template <typename T>
std::vector<dts::scheduler::task_t> AssetListLoader::callbacks(AssetList<T> list) const {
	std::vector<dts::scheduler::task_t> ret;
	for (auto& kvp : list.map) {
		if (!skip<T>(kvp.first)) {
			ret.push_back([s = &store(), kvp = std::move(kvp)]() mutable { s->add(std::move(kvp.first), kvp.second()); });
		}
	}
	list.map.clear();
	return ret;
}

template <typename T>
void AssetListLoader::load_(AssetLoadList<T> list) const {
	for (auto& [id, data] : list.map) {
		if (!skip<T>(id)) { store().load<T>(id, std::move(data)); }
	}
}

template <typename T>
void AssetListLoader::load_(AssetList<T> list) const {
	for (auto& [id, cb] : list.map) {
		if (!skip<T>(id)) { store().add(id, cb()); }
	}
}
} // namespace le
