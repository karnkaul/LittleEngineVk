#include <engine/render/material.hpp>
#include <graphics/context/command_buffer.hpp>
#include <graphics/pipeline.hpp>
#include <graphics/texture.hpp>

namespace le {
void MatTextured::write(std::size_t idx) {
	ENSURE(pPipe && diffuse.tex, "Null pipeline/texture view");
	pPipe->shaderInput().update(*diffuse.tex, diffuse.sb.set, diffuse.sb.bind, idx);
}

void MatTextured::bind(graphics::CommandBuffer const& cb, std::size_t idx) const {
	cb.bindSet(pPipe->layout(), pPipe->shaderInput().set(diffuse.sb.set).index(idx));
}

void MatSkybox::write(std::size_t idx) {
	ENSURE(pPipe && cubemap.tex, "Null pipeline/texture view");
	pPipe->shaderInput().update(*cubemap.tex, cubemap.sb.set, cubemap.sb.bind, idx);
}
} // namespace le
