#pragma once
#include <ktl/async/kfunction.hpp>
#include <levk/core/services.hpp>
#include <levk/engine/assets/asset_store.hpp>
#include <levk/graphics/draw_primitive.hpp>
#include <levk/graphics/render/draw_list.hpp>

namespace le {
class PrimitiveProvider {
  public:
	using DrawPrimitive = graphics::DrawPrimitive;
	using MatTexType = graphics::MatTexType;

	inline static Hash const default_material_v = "materials/bp/default";

	PrimitiveProvider() = default;
	PrimitiveProvider(Hash meshPrimitiveURI, Hash materialURI = default_material_v);

	void texture(MatTexType type, Hash uri) { m_textures[type] = uri; }
	Hash texture(MatTexType type) const { return m_textures[type]; }
	Hash meshPrimitiveURI() const { return m_meshURI; }
	Hash materialURI() const { return m_materialURI; }

	bool ready(AssetStore const& store) const;
	bool addDrawPrimitives(AssetStore const& store, graphics::DrawList& out, glm::mat4 const& matrix);

  private:
	graphics::TMatTexArray<Hash> m_textures{};
	Hash m_meshURI{};
	Hash m_materialURI{};
};

// class DynamicMeshView {
//   public:
// 	using GetMesh = ktl::kfunction<MeshView()>;

// 	static DynamicMeshView make(GetMesh&& getMesh);
// 	template <MeshViewAPI T>
// 	static DynamicMeshView make(T const* source);

// 	bool active() const noexcept { return m_getMesh.has_value(); }
// 	MeshView mesh() const { return m_getMesh ? m_getMesh() : MeshView{}; }

//   private:
// 	GetMesh m_getMesh;
// };

// impl

// template <MeshViewAPI T>
// DynamicMeshView DynamicMeshView::make(T const* source) {
// 	EXPECT(source);
// 	return make([source]() { return source->mesh(); });
// }
} // namespace le
