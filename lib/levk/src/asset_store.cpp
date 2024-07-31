#include <djson/json.hpp>
#include <levk/asset_store.hpp>
#include <levk/assets/mesh_asset.hpp>
#include <levk/core/string_hash.hpp>

namespace levk {
namespace {
class AssetStore : public IAssetStore {
  public:
	explicit AssetStore(NotNull<IVfs const*> vfs, NotNull<IEngine*> engine) : IAssetStore(vfs, engine) {}

  private:
	[[nodiscard]] auto get_loaded_asset(std::string_view const uri) const -> Ptr<IAsset> final {
		auto lock = std::scoped_lock{m_mutex};
		if (auto const it = m_map.find(uri); it != m_map.end()) { return it->second.get(); }
		return {};
	}

	void store_asset(std::string_view const uri, std::unique_ptr<IAsset> asset) final {
		++m_sentinel;
		auto lock = std::scoped_lock{m_mutex};
		m_map.insert_or_assign(std::string{uri}, std::move(asset));
	}

	[[nodiscard]] auto get_asset_count() const -> std::size_t final {
		auto lock = std::scoped_lock{m_mutex};
		return m_map.size();
	}

	void clear_stored() final {
		++m_sentinel;
		auto lock = std::scoped_lock{m_mutex};
		m_map.clear();
	}

	void fill_uris(std::vector<std::string>& out, Ptr<Sentinel> out_sentinel) const final {
		auto lock = std::scoped_lock{m_mutex};
		out.reserve(out.size() + m_map.size());
		for (auto const& [uri, _] : m_map) { out.push_back(uri); }
		if (out_sentinel != nullptr) { *out_sentinel = get_sentinel(); }
	}

	[[nodiscard]] auto get_sentinel() const -> Sentinel final { return Sentinel{m_sentinel.load()}; }

	std::unordered_map<std::string, std::unique_ptr<IAsset>, StringHash, std::equal_to<>> m_map;
	mutable std::mutex m_mutex{};
	std::atomic<std::underlying_type_t<Sentinel>> m_sentinel{};
};
} // namespace

auto IAssetStore::read_asset_type_name(dj::Json const& json) -> std::string { return json["type_name"].as<std::string>(); }

auto IAssetStore::get_asset_type_name(std::string_view const uri) const -> std::string { return read_asset_type_name(get_vfs().load_json(uri)); }
} // namespace levk

auto levk::create_asset_store(NotNull<IVfs const*> vfs, NotNull<IEngine*> engine) -> std::unique_ptr<IAssetStore> {
	return std::make_unique<AssetStore>(vfs, engine);
}
