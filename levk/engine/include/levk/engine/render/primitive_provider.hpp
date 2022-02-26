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

	inline static Hash const default_material_v = "materials/bp/default";

	PrimitiveProvider() = default;
	PrimitiveProvider(Hash meshPrimitiveURI, Hash materialURI = default_material_v, Hash textureRefsURI = {});

	Hash meshPrimitiveURI() const { return m_meshURI; }
	Hash materialURI() const { return m_materialURI; }
	Hash matTexRefsURI() const { return m_texRefsURI; }

	bool addDrawPrimitives(AssetStore const& store, graphics::DrawList& out, glm::mat4 const& matrix);

  private:
	Hash m_meshURI{};
	Hash m_materialURI{};
	Hash m_texRefsURI{};
};

class PrimitiveGenerator {
  public:
	using AddPrims = ktl::kfunction<void(graphics::DrawList&, glm::mat4 const&)>;

	PrimitiveGenerator() = default;
	PrimitiveGenerator(AddPrims&& addPrims) noexcept : m_addPrims(std::move(addPrims)) {}

	template <graphics::DrawPrimitiveAPI T>
	static PrimitiveGenerator make(T* source) {
		return PrimitiveGenerator([source](graphics::DrawList& out, glm::mat4 const& matrix) {
			if (source) { out.add(*source, matrix); }
		});
	}

	explicit operator bool() const { return m_addPrims.has_value(); }

	void addDrawPrimitives(graphics::DrawList& out, glm::mat4 const& matrix) {
		if (m_addPrims) { m_addPrims(out, matrix); }
	}

  private:
	AddPrims m_addPrims;
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
