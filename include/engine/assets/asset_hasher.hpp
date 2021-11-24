#pragma once
#include <engine/assets/asset.hpp>
#include <functional>

namespace le {
template <typename T>
struct AssetHasher {
	std::size_t operator()(Asset<T> const& asset) const noexcept { return std::hash<std::string_view>{}(asset.m_uri); }
};
} // namespace le
