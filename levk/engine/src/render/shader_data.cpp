#include <levk/engine/render/shader_data.hpp>
#include <levk/graphics/material_data.hpp>
#include <levk/graphics/render/camera.hpp>

namespace le {
ShaderSceneView ShaderSceneView::make(graphics::Camera const& camera, glm::vec2 const extent) noexcept {
	return {camera.view(), camera.perspective(extent), camera.ortho(extent), {camera.position, 1.0f}};
}

ShaderMaterial ShaderMaterial::make(graphics::BPMaterialData const& mtl) noexcept {
	ShaderMaterial ret;
	ret.tint = {static_cast<glm::vec3 const&>(mtl.Tf.toVec4()), mtl.d};
	ret.ambient = mtl.Ka.toVec4();
	ret.diffuse = mtl.Kd.toVec4();
	ret.specular = mtl.Ks.toVec4();
	return ret;
}
} // namespace le
