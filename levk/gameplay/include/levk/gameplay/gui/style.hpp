#pragma once
#include <glm/vec2.hpp>
#include <levk/core/hash.hpp>
#include <levk/core/utils/error.hpp>
#include <levk/engine/render/material.hpp>
#include <levk/gameplay/gui/interact.hpp>
#include <levk/graphics/material_data.hpp>
#include <levk/graphics/rgba.hpp>
#include <unordered_map>

namespace le::gui {
struct TextStyle {
	glm::vec2 pivot = {};
	graphics::RGBA colour = colours::black;
	u32 height = 40U;
};

constexpr InteractStyle<Material> defaultQuadInteractStyle() noexcept;
constexpr InteractStyle<graphics::BPMaterialData> defaultQuadInteractStyle2() noexcept;

struct BaseStyle {
	TextStyle text;
	graphics::RGBA background = colours::white;
};

struct WidgetStyle {
	InteractStyle<Material> quad = defaultQuadInteractStyle();
	InteractStyle<graphics::BPMaterialData> quad2 = defaultQuadInteractStyle2();
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
		ENSURE(id != Hash(), "Invalid id");
		return s_map[id];
	}

	static Style& getDefault() noexcept { return s_default; }

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

constexpr InteractStyle<graphics::BPMaterialData> defaultQuadInteractStyle2() noexcept {
	InteractStyle<graphics::BPMaterialData> ret;
	ret.at(InteractStatus::eHover).Tf = colours::cyan;
	ret.at(InteractStatus::eHold).Tf = colours::yellow;
	return ret;
}
} // namespace le::gui
