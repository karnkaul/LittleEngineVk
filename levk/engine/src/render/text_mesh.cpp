#include <levk/engine/render/text_mesh.hpp>
#include <levk/graphics/font/font.hpp>

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

TextMesh::TextMesh(not_null<graphics::VRAM*> vram) noexcept : m_obj(Obj::make(vram)) {}

TextMesh::TextMesh(not_null<Font*> font) noexcept : m_obj(Obj::make(font->m_vram)), m_font(font) {}

MeshView TextMesh::mesh() const {
	if (!m_font) { return {}; }
	return m_info.visit(ktl::koverloaded{
		[&](graphics::Geometry const& geom) { return m_obj.mesh(*m_font, geom, m_colour); },
		[&](TextMesh::Line const& line) {
			if (line.line.empty()) { return MeshView{}; }
			return m_obj.mesh(*m_font, line.line, m_colour, line.layout);
		},
	});
}
} // namespace le
