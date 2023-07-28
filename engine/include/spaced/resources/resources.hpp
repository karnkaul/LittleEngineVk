#pragma once
#include <spaced/core/mono_instance.hpp>
#include <spaced/core/ptr.hpp>
#include <spaced/error.hpp>
#include <spaced/resources/asset.hpp>
#include <spaced/vfs/uri.hpp>
#include <concepts>
#include <format>
#include <mutex>
#include <unordered_map>

namespace spaced {
class Resources : public MonoInstance<Resources> {
  public:
	static auto try_load(Uri const& uri, Asset& out) -> bool;

	auto store(Uri uri, std::unique_ptr<Asset> asset) -> Ptr<Asset>;
	auto reload(Uri uri) -> Ptr<Asset>;

	auto contains(Uri const& uri) const -> bool;
	auto find(Uri const& uri) const -> Ptr<Asset>;

	template <std::derived_from<Asset> Type>
	auto load(Uri const& uri) -> Ptr<Type> {
		if (auto* ret = find<Type>(uri)) { return ret; }
		auto asset = std::make_unique<Type>();
		if (try_load(uri, *asset)) {
			auto* ret = asset.get();
			set(uri, std::move(asset), "loaded");
			return ret;
		}
		return {};
	}

	template <std::derived_from<Asset> Type>
	auto find(Uri const& uri) const -> Ptr<Type> {
		return dynamic_cast<Type*>(find(uri));
	}

	template <std::derived_from<Asset> Type>
	auto get(Uri const& uri) const -> Asset& {
		auto* ret = find<Type>(uri);
		if (!ret) { throw Error{std::format("Failed to find Asset: '{}'", uri.value())}; }
		return *ret;
	}

	auto clear() -> void;

  private:
	auto set(Uri uri, std::unique_ptr<Asset> asset, std::string_view log_suffix) -> Ptr<Asset>;

	std::unordered_map<Uri, std::unique_ptr<Asset>, Uri::Hasher> m_assets{};
	mutable std::mutex m_mutex{};
};
} // namespace spaced
