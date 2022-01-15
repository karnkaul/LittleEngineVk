#pragma once
#include <levk/window/event.hpp>

namespace le::window {
template <typename T>
struct Box {
	T t{};
};
template <typename T>
Box(T) -> Box<T>;

struct EventStorage {
	using Drop = std::vector<std::string>;
	ktl::fixed_vector<Drop, 8> drops;
};

struct Event::Builder {
	Event operator()(Key const key) const noexcept {
		Event ret;
		ret.m_type = Type::eKey;
		ret.m_payload.key = key;
		return ret;
	}
	Event operator()(Button const button) const noexcept {
		Event ret;
		ret.m_type = Type::eMouseButton;
		ret.m_payload.button = button;
		return ret;
	}
	Event operator()(Box<std::uint32_t> codepoint) const noexcept {
		Event ret;
		ret.m_type = Type::eText;
		ret.m_payload.u32 = codepoint.t;
		return ret;
	}
	Event operator()(decltype(EventStorage::drops) const& drops) noexcept {
		EXPECT(!drops.empty());
		Event ret;
		ret.m_type = Type::eFileDrop;
		ret.m_payload.index = drops.size() - 1U;
		return ret;
	}
	Event operator()(Type const type, glm::uvec2 const uv2) const noexcept {
		Event ret;
		ret.m_type = type;
		ret.m_payload.uv2 = {uv2.x, uv2.y};
		return ret;
	}
	Event operator()(Type const type, glm::ivec2 const iv2) const noexcept {
		Event ret;
		ret.m_type = type;
		ret.m_payload.iv2 = {iv2.x, iv2.y};
		return ret;
	}
	Event operator()(Type const type, glm::dvec2 const dv2) const noexcept {
		Event ret;
		ret.m_type = type;
		ret.m_payload.dv2 = {dv2.x, dv2.y};
		return ret;
	}
	Event operator()(Type const type, Box<bool> const b) const noexcept {
		Event ret;
		ret.m_type = type;
		ret.m_payload.b = b.t;
		return ret;
	}
};
} // namespace le::window
