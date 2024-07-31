#pragma once
#include <levk/asset.hpp>
#include <levk/core/ptr.hpp>
#include <levk/core/time.hpp>
#include <levk/engine.hpp>
#include <levk/logger.hpp>
#include <levk/vfs.hpp>

namespace levk {
/// \brief Concept for an asset type that can be loaded by AssetStore.
template <typename Type>
concept LoadableAssetT = std::derived_from<Type, IAsset> && std::is_default_constructible_v<Type>;

/// \brief Opaque interface for storage and retreival of concrete IAssets.
class IAssetStore : public Polymorphic {
  public:
	using LoadInfo = IAsset::LoadInfo;

	enum struct Sentinel : std::int64_t {};

	explicit IAssetStore(NotNull<IVfs const*> vfs, NotNull<IEngine*> engine) : m_vfs(vfs), m_engine(engine) {}

	[[nodiscard]] auto get_vfs() const -> IVfs const& { return *m_vfs; }
	[[nodiscard]] auto get_engine() const -> IEngine const& { return *m_engine; }

	/// \brief Load an asset.
	/// \param info LoadInfo for the asset.
	/// \returns Pointer to stored/loaded asset, nullptr if loading failed.
	/// Can be called from multiple threads but NOT for the same info.uri!
	template <LoadableAssetT Type>
	auto load(LoadInfo const& info) -> Ptr<Type> {
		if (info.uri.empty()) { return {}; }
		if (!info.reload) {
			if (auto* ret = get<Type>(info.uri)) { return ret; }
		}
		return try_load<Type>(info);
	}

	/// \brief Load an asset.
	/// \param uri URI of asset data.
	/// \returns Pointer to stored/loaded asset, nullptr if loading failed.
	/// Can be called from multiple threads but NOT for the same uri!
	template <LoadableAssetT Type>
	auto load(std::string_view const uri) -> Ptr<Type> {
		return load<Type>(LoadInfo{.uri = uri});
	}

	/// \brief Get a stored asset.
	/// \param uri URI of stored asset.
	/// \returns Pointer to stored asset, nullptr if loading failed.
	/// Can be called from multiple threads.
	template <typename Type>
	[[nodiscard]] auto get(std::string_view const uri) const -> Ptr<Type> {
		auto* asset = get_loaded_asset(uri);
		if constexpr (std::same_as<Type, IAsset>) { return asset; }
		if (asset == nullptr) { return nullptr; }

		auto* ret = dynamic_cast<Type*>(asset);
		if (ret == nullptr) {
			m_log.warn("Asset type mismatch for '{}': stored asset is a {}", uri, asset->get_type_name());
			return nullptr;
		}

		return ret;
	}

	/// \brief Store an asset.
	/// \param uri URI to store as.
	/// \param asset Asset to store.
	/// Can be called from multiple threads but NOT for the same uri!
	void store(std::string_view uri, std::unique_ptr<IAsset> asset) {
		if (!asset) { return; }
		m_log.info("{} stored: '{}'", asset->get_type_name(), uri);
		store_asset(uri, std::move(asset));
	}

	/// \brief Get the number o stored assets.
	/// Can be called from multiple threads.
	[[nodiscard]] auto get_count() const -> std::size_t { return get_asset_count(); }
	/// \brief Clear all stored assets.
	void clear() { clear_stored(); }

	/// \brief Get the type name of the JSON asset.
	/// \param json JSON to read from.
	/// \returns Value of field "type_name", if not found then empty string.
	[[nodiscard]] static auto read_asset_type_name(dj::Json const& json) -> std::string;

	/// \brief Get the type name of an asset given its JSON URI.
	/// \param uri URI to asset JSON.
	/// \returns Value of field "type_name" in JSON, if not found then empy string.
	/// Can be called from multiple threads.
	[[nodiscard]] auto get_asset_type_name(std::string_view uri) const -> std::string;

	/// \brief Append currently stored/loaded asset URIs to passed buffer.
	/// \param out Vector to append to.
	virtual void fill_uris(std::vector<std::string>& out, Ptr<Sentinel> out_sentinel = {}) const = 0;

	/// \brief Get a sentinel value that changes if the stored assets is modified.
	/// \returns Current sentinel.
	[[nodiscard]] virtual auto get_sentinel() const -> Sentinel = 0;

  protected:
	[[nodiscard]] virtual auto get_loaded_asset(std::string_view uri) const -> Ptr<IAsset> = 0;
	virtual void store_asset(std::string_view uri, std::unique_ptr<IAsset> asset) = 0;
	[[nodiscard]] virtual auto get_asset_count() const -> std::size_t = 0;
	virtual void clear_stored() = 0;

	Logger m_log{"AssetStore"};

  private:
	template <typename Type>
	[[nodiscard]] auto try_load(LoadInfo const& info) -> Ptr<Type> {
		auto t = std::make_unique<Type>();
		auto& asset = static_cast<IAsset&>(*t);
		auto const load_start = Clock::now();
		if (!asset.load(*this, info)) {
			m_log.warn("Failed to load {}: '{}'", asset.get_type_name(), info.uri);
			return {};
		}
		auto const load_time = std::chrono::duration_cast<std::chrono::milliseconds>(Clock::now() - load_start);

		auto* ret = t.get();
		store_asset(info.uri, std::move(t));
		m_log.info("{} loaded: '{}' ({}ms)", asset.get_type_name(), info.uri, load_time.count());
		return ret;
	}

	NotNull<IVfs const*> m_vfs;
	NotNull<IEngine*> m_engine;
};

[[nodiscard]] auto create_asset_store(NotNull<IVfs const*> vfs, NotNull<IEngine*> engine) -> std::unique_ptr<IAssetStore>;
} // namespace levk
