#include <graphics/font/font.hpp>
#include <levk/engine/render/text_mesh.hpp>

namespace le {
TextMesh::Obj TextMesh::Obj::make(not_null<graphics::VRAM*> vram) { return Obj{{vram, graphics::MeshPrimitive::Type::eDynamic}, {}}; }

MeshView TextMesh::Obj::mesh(Font& font, graphics::Geometry const& geometry, RGBA const colour) {
	primitive.construct(geometry);
	material.Tf = colour;
	material.map_Kd = &font.atlas().texture();
	return MeshObj{&primitive, &material};
}

MeshView TextMesh::Obj::mesh(graphics::Font& font, std::string_view line, RGBA const colour, TextLayout const& layout) {
	graphics::Geometry geom;
	graphics::Font::PenInfo const info{layout.origin, layout.scale, layout.lineSpacing, &geom};
	graphics::Font::Pen pen(&font, info);
	pen.writeText(line, &layout.pivot);
	return mesh(font, geom, colour);
}

TextMesh::TextMesh(not_null<Font*> font) noexcept : m_obj(Obj::make(font->m_vram)), m_font(font) {}

MeshView TextMesh::mesh() const {
	if (auto geom = m_info.get_if<graphics::Geometry>()) { return m_obj.mesh(*m_font, *geom, m_colour); }
	if (auto line = m_info.get<Line>(); !line.line.empty()) { return m_obj.mesh(*m_font, line.line, m_colour, line.layout); }
	return {};
}
} // namespace le
