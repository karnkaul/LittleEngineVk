#pragma once
#include <unordered_map>
#include <core/ensure.hpp>
#include <core/hash.hpp>
#include <engine/gui/interact.hpp>
#include <engine/render/material.hpp>
#include <graphics/render/rgba.hpp>
#include <graphics/text_factory.hpp>

namespace le::gui {
struct TextStyle {
	glm::vec2 align = {};
	graphics::RGBA colour = colours::black;
	graphics::TextFactory::Size size = 40U;
};

constexpr InteractStyle<Material> defaultQuadInteractStyle() noexcept;

struct BaseStyle {
	TextStyle text;
	graphics::RGBA background = colours::white;
};

struct WidgetStyle {
	InteractStyle<Material> quad = defaultQuadInteractStyle();
	InteractStyle<TextStyle> text;
};

struct Style {
	BaseStyle base;
	WidgetStyle widget;
};

class Styles {
  public:
	static Style const& get(Hash id) noexcept {
		if (id != Hash()) {
			if (auto it = s_map.find(id); it != s_map.end()) { return it->second; }
		}
		return s_default;
	}

	static Style& getOrInsert(Hash id) {
		ensure(id != Hash(), "Invalid id");
		return s_map[id];
	}

	static Style& getDefault() noexcept { return s_default; }

	static void setup(graphics::TextFactory& out_factory, Style const* style = {}) noexcept {
		if (!style) { style = &getDefault(); }
		out_factory.size = style->base.text.size;
		out_factory.align = style->base.text.align;
		out_factory.colour = style->base.text.colour;
	}

	static void setup(graphics::TextFactory& out_factory, Hash style) noexcept { setup(out_factory, &get(style)); }

  private:
	inline static std::unordered_map<Hash, Style> s_map;
	inline static Style s_default;
};

// impl

constexpr InteractStyle<Material> defaultQuadInteractStyle() noexcept {
	InteractStyle<Material> ret;
	ret.at(InteractStatus::eHover).Tf = colours::cyan;
	ret.at(InteractStatus::eHold).Tf = colours::yellow;
	return ret;
}
} // namespace le::gui
