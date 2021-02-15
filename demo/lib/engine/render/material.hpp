#pragma once
#include <engine/render/interface.hpp>

namespace le {
namespace graphics {
class Pipeline;
class Texture;
} // namespace graphics

struct Layer {
	std::string_view name = "Default";
	s32 order = 0;
};

struct SetBind {
	u32 set = 0;
	u32 bind = 0;
};

struct MatBlank : IMaterial {
	graphics::Pipeline* pPipe{};
	Layer layer;

	void write(std::size_t) override;
	void bind(graphics::CommandBuffer const& cb, std::size_t idx) const override;
};

struct MatTextured : MatBlank {
	struct {
		graphics::Texture const* tex = {};
		SetBind sb = {2, 0};
	} diffuse;

	void write(std::size_t idx) override;
	void bind(graphics::CommandBuffer const& cb, std::size_t idx) const override;
};

struct MatUI : MatTextured {
	MatUI() noexcept;
};

struct MatSkybox : MatBlank {
	struct {
		graphics::Texture const* tex = {};
		SetBind sb = {0, 1};
	} cubemap;

	MatSkybox() noexcept;

	void write(std::size_t) override;
};

// impl

inline void MatBlank::write(std::size_t) {
}
inline void MatBlank::bind(graphics::CommandBuffer const&, std::size_t) const {
}

inline MatUI::MatUI() noexcept {
	layer = {"ui", 100};
}

inline MatSkybox::MatSkybox() noexcept {
	layer = {"skybox", -10};
}
} // namespace le
