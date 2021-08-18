#include <engine/render/bitmap_text.hpp>
#include <engine/render/model.hpp>
#include <engine/scene/prop_provider.hpp>
#include <graphics/mesh.hpp>

namespace le {
Span<Prop const> PropExtractor<Model>::operator()(Model const& model) const noexcept { return model.props(); }

Span<Prop const> PropExtractor<Text2D>::operator()(Text2D const& text) const noexcept { return text.props(); }
} // namespace le
