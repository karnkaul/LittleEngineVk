#include <engine/render/text_mesh.hpp>
#include <graphics/font/font.hpp>

namespace le {
TextMesh::Obj TextMesh::Obj::make(not_null<graphics::VRAM*> vram) { return Obj{{vram, graphics::MeshPrimitive::Type::eDynamic}, {}}; }

MeshView TextMesh::Obj::mesh(graphics::Font& font, std::string_view line, Info const& info) {
	graphics::Geometry geom;
	graphics::Font::Pen pen(&font, &geom, info.origin, info.scale);
	pen.write(line, info.pivot);
	primitive.construct(geom);
	material.Tf = info.colour;
	material.map_Kd = &font.atlas().texture();
	return MeshObj{&primitive, &material};
}

TextMesh::TextMesh(not_null<Font*> font) noexcept : m_obj(Obj::make(font->m_vram)), m_font(font) {}

MeshView TextMesh::mesh() const {
	if (!m_line.empty()) { return m_obj.mesh(*m_font, m_line, m_info); }
	return {};
}
} // namespace le
