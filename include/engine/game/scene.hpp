#pragma once
#include <core/ec/types.hpp>
#include <core/ec_registry.hpp>
#include <core/traits.hpp>
#include <core/transform.hpp>
#include <engine/game/input.hpp>
#include <engine/gfx/camera.hpp>
#include <engine/gfx/light.hpp>
#include <engine/gfx/pipeline.hpp>
#include <engine/gfx/screen_rect.hpp>
#if defined(LEVK_EDITOR)
#include <engine/game/editor_types.hpp>
#endif

namespace le {
///
/// \brief Wrapper for Entity + Transform
///
struct Prop final {
	ecs::Entity entity;
	Transform* pTransform;

	constexpr Prop() noexcept;
	constexpr Prop(ecs::Entity entity, Transform& transform) noexcept;

	///
	/// \brief Check whether Prop points to a valid pair of Entity/Transform
	///
	constexpr bool valid() const noexcept;

	///
	/// \brief Obtain the transform (must be valid!)
	///
	Transform const& transform() const;
	///
	/// \brief Obtain the transform (must be valid!)
	///
	Transform& transform();

	constexpr operator ecs::Entity() const noexcept;
};

///
/// \brief Return type wrapper for `spawnProp<T...>()`
///
template <typename... T>
struct TProp {
	using type = TProp;
	using Components = ecs::Components<T&...>;

	Prop prop;
	Components components;

	constexpr operator Prop() const noexcept;
};

template <>
struct TProp<> {
	using type = Prop;
};

template <typename... T>
using TProp_t = typename TProp<T...>::type;

class GameScene final {
  public:
	struct Desc final {
		enum class Flag : s8 {
			///
			/// \brief UI follows screen size and aspect ratio
			///
			eDynamicUI,
			///
			/// \brief UI clipped to match its aspect ratio
			///
			eScissoredUI,
			eCOUNT_
		};
		using Flags = kt::enum_flags<Flag>;

		gfx::Camera defaultCam;
		gfx::Camera* pCustomCam = nullptr;
		std::vector<gfx::DirLight> dirLights;
		io::Path skyboxCubemapID;
		f32 orthoDepth = 2.0f;
		gfx::Pipeline pipe3D;
		gfx::Pipeline pipeUI;
		glm::vec2 clearDepth = {1.0f, 0.0f};
		Colour clearColour = colours::black;
		Flags flags = Flag::eDynamicUI;

		gfx::Camera const& camera() const;
		gfx::Camera& camera();
	};

	using EntityMap = std::unordered_map<Ref<Transform>, ecs::Entity>;

#if defined(LEVK_EDITOR)
	static constexpr bool s_bParentToRoot = true;
#else
	static constexpr bool s_bParentToRoot = false;
#endif

	EntityMap m_entityMap;
	Transform m_sceneRoot;

	std::string m_name;
	ecs::Registry m_registry;

#if defined(LEVK_EDITOR)
	editor::PerFrame m_editorData;
#endif

  public:
	///
	/// \brief Obtain (a reference to) the scene descriptor
	///
	/// A Desc will be added to the Registry if none exist
	///
	Desc& desc();
	///
	/// \brief Obtain (a reference to) the main scene camera
	///
	/// A Desc will be added to the Registry if none exist
	///
	gfx::Camera& mainCamera();

	///
	/// \brief Create a new Prop
	///
	/// Transform will be parented to scene root if LEVK_EDITOR is defined
	///
	template <typename T, typename... Args>
	TProp_t<T> spawnProp(std::string name, Transform* pParent = nullptr, Args&&... args);
	///
	/// \brief Create a new Prop
	///
	/// Transform will be parented to scene root if LEVK_EDITOR is defined
	template <typename... T, typename = require<!size_eq_v<1, T...>>>
	TProp_t<T...> spawnProp(std::string name, Transform* pParent = nullptr);
	///
	/// \brief Reparent to another Transform / unparent if parented
	///
	/// Unparenting will parent to scene root if LEVK_EDITOR is defined
	bool reparent(Prop prop, Transform* pParent = nullptr);
	///
	/// \brief Destroy one or more props
	///
	void destroy(Span<Prop> props);

	void reset();

  private:
	void boltOnRoot(Prop prop);
};

inline constexpr Prop::Prop() noexcept : pTransform(nullptr) {
}
inline constexpr Prop::Prop(ecs::Entity entity, Transform& transform) noexcept : entity(entity), pTransform(&transform) {
}
inline constexpr Prop::operator ecs::Entity() const noexcept {
	return entity;
}
inline constexpr bool Prop::valid() const noexcept {
	return pTransform != nullptr && entity.id.payload != ecs::ID::null;
}
inline Transform const& Prop::transform() const {
	ENSURE(pTransform, "Null Transform!");
	return *pTransform;
}

inline Transform& Prop::transform() {
	ENSURE(pTransform, "Null Transform!");
	return *pTransform;
}

template <typename... T>
constexpr TProp<T...>::operator Prop() const noexcept {
	return prop;
}

template <typename T, typename... Args>
TProp_t<T> GameScene::spawnProp(std::string name, Transform* pParent, Args&&... args) {
	ecs::Registry& reg = m_registry;
	auto ec = reg.template spawn<T, Args...>(std::move(name), std::forward<Args>(args)...);
	auto pT = reg.template attach<Transform>(ec);
	ENSURE(pT, "Invariant violated!");
	Prop prop{ec, *pT};
	if constexpr (s_bParentToRoot) {
		boltOnRoot(prop);
	}
	if (pParent) {
		pT->parent(pParent);
	}
	return {prop, std::move(ec.components)};
}

template <typename... T, typename>
TProp_t<T...> GameScene::spawnProp(std::string name, Transform* pParent) {
	ecs::Registry& reg = m_registry;
	auto ec = reg.template spawn<T...>(std::move(name));
	auto pT = reg.template attach<Transform>(ec);
	ENSURE(pT, "Invariant violated!");
	Prop prop{ec, *pT};
	if constexpr (s_bParentToRoot) {
		boltOnRoot(prop);
	}
	if (pParent) {
		pT->parent(pParent);
	}
	if constexpr (sizeof...(T) > 0) {
		return {prop, std::move(ec.components)};
	} else {
		return prop;
	}
}
} // namespace le
