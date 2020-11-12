#pragma once
#include <core/time.hpp>
#include <engine/game/scene_builder.hpp>

namespace le::engine {
///
/// \brief Interface for engine to drive frames
///
struct Driver {
	///
	/// \brief Frame update
	/// \param dt delta time since previous call
	///
	virtual void tick(Time_s dt) = 0;
	///
	/// \brief Frame build
	/// \returns SceneBuilder (to build scene from game state registry)
	///
	virtual inline SceneBuilder const& builder() const {
		static SceneBuilder const s_default;
		return s_default;
	}
};
} // namespace le::engine
