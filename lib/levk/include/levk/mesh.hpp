#pragma once
#include <levk/core/not_null.hpp>
#include <levk/primitive.hpp>
#include <levk/skeleton.hpp>
#include <string>
#include <vector>

namespace levk {
/// \brief Abstract base for meshes.
class IMesh : public Polymorphic {
  public:
	[[nodiscard]] virtual auto get_primitives() const -> PrimitivesView = 0;

	std::string name{};
};

/// \brief Base class template for meshes.
template <std::derived_from<IPrimitive> Type>
class BasicMesh : public IMesh {
  public:
	[[nodiscard]] auto get_primitives() const -> PrimitivesView final { return primitives; }

	std::vector<NotNull<IPrimitive const*>> primitives{};
};

/// \brief Static Mesh.
class StaticMesh : public BasicMesh<IStaticPrimitive> {};

/// \brief Skinned Mesh.
class SkinnedMesh : public BasicMesh<ISkinnedPrimitive> {
  public:
	SkinnedMesh() = default;

	explicit SkinnedMesh(Skeleton skeleton) : m_skeleton(std::move(skeleton)) {}

	/// \brief Get the skeleton used by this mesh.
	[[nodiscard]] auto get_skeleton() const -> Skeleton const& { return m_skeleton; }

  private:
	Skeleton m_skeleton;
};
} // namespace levk
