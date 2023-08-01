#pragma once
#include <spaced/core/mono_instance.hpp>
#include <spaced/core/ptr.hpp>
#include <spaced/error.hpp>
#include <spaced/resources/asset.hpp>
#include <spaced/vfs/uri.hpp>
#include <concepts>
#include <format>
#include <future>
#include <mutex>
#include <unordered_map>

namespace spaced {
class Resources : public MonoInstance<Resources> {
  public:
	static auto try_load(Uri const& uri, Asset& out) -> bool;

	auto store(Uri const& uri, std::unique_ptr<Asset> asset) -> Ptr<Asset>;
	auto reload(std::string_view uri) -> Ptr<Asset>;

	auto contains(std::string_view uri) const -> bool;
	auto find(std::string_view uri) const -> Ptr<Asset>;

	template <std::derived_from<Asset> Type>
	auto load(std::string_view const uri) -> Ptr<Type> {
		if (auto* ret = find<Type>(uri)) { return ret; }
		auto const uri_ = Uri{std::string{uri}};
		auto asset = std::make_unique<Type>();
		if (try_load(uri_, *asset)) {
			auto* ret = asset.get();
			set(uri_, std::move(asset));
			return ret;
		}
		return {};
	}

	template <std::derived_from<Asset> Type>
	auto load_async(Uri uri, std::launch launch = std::launch::async) -> std::future<Ptr<Type>> {
		return std::async(launch, [this, uri = std::move(uri)] { return load<Type>(uri); });
	}

	template <std::derived_from<Asset> Type>
	auto find(std::string_view const uri) const -> Ptr<Type> {
		return dynamic_cast<Type*>(find(uri));
	}

	template <std::derived_from<Asset> Type>
	auto get(std::string_view const uri) const -> Asset& {
		if (auto* ret = find<Type>(uri)) { return *ret; }
		throw Error{std::format("Failed to find Asset: '{}'", uri)};
	}

	template <std::derived_from<Asset> Type>
	[[nodiscard]] static auto is_ready(std::future<Ptr<Type>> const& future) -> bool {
		if (!future.valid()) { return false; }
		return future.wait_for(std::chrono::seconds{}) == std::future_status::ready;
	}

	auto clear() -> void;

  private:
	auto set(Uri uri, std::unique_ptr<Asset> asset) -> Ptr<Asset>;

	std::unordered_map<Uri, std::unique_ptr<Asset>, Uri::Hasher, std::equal_to<>> m_assets{};
	mutable std::mutex m_mutex{};
};
} // namespace spaced
