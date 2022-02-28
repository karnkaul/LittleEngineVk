#pragma once
#include <ktl/async/kfunction.hpp>
#include <levk/engine/assets/asset_store.hpp>
#include <chrono>

namespace le {
class AssetMonitor {
  public:
	using FTime = std::chrono::file_clock::time_point;
	using Sign = AssetStore::Sign;

	template <typename T>
	using OnModified = ktl::kfunction<bool(T&)>;

	template <typename T>
	void attach(Hash uri, Span<io::Path const> paths, OnModified<T> onModified);
	void detach(Hash uri) { m_monitors.erase(uri); }
	void clear() { m_monitors.clear(); }

	std::size_t update(AssetStore const& store);

  private:
	struct Base;
	template <typename T>
	struct Monitor;

	std::unordered_map<Hash, std::unique_ptr<Base>> m_monitors;
};

// impl

struct AssetMonitor::Base {
	struct Entry {
		std::string path;
		FTime lastModified;
	};

	std::vector<Entry> entries;

	Base(Span<io::Path const> paths);
	virtual ~Base() = default;
	bool modified();
	virtual bool update(AssetStore const& store, Hash uri) = 0;
};

template <typename T>
struct AssetMonitor::Monitor : Base {
	OnModified<T> onModified;
	Monitor(Span<io::Path const> paths, OnModified<T>&& onModified) : Base(paths), onModified(std::move(onModified)) {}
	bool update(AssetStore const& store, Hash uri) override {
		auto t = store.find<T>(uri);
		if (t && modified() && onModified(*t)) {
			logI(LC_EndUser, "[Assets] [{}] modified", store.uri<T>(uri));
			return true;
		}
		return false;
	}
};

template <typename T>
void AssetMonitor::attach(Hash uri, Span<io::Path const> paths, OnModified<T> onModified) {
	if (uri != Hash() && !paths.empty() && onModified) { m_monitors.insert_or_assign(uri, std::make_unique<Monitor<T>>(paths, std::move(onModified))); }
}
} // namespace le
