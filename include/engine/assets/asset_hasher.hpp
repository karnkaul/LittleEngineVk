#pragma once
#include <functional>
#include <engine/assets/asset.hpp>

namespace le {
template <typename T>
struct AssetHasher {
	std::size_t operator()(Asset<T> const& asset) const noexcept { return std::hash<std::string_view>{}(asset.m_id); }
};
} // namespace le
