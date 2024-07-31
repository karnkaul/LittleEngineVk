#pragma once
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/mat4x4.hpp>
#include <levk/core/radians.hpp>

namespace levk {
inline constexpr auto right_v = glm::vec3{1.0f, 0.0f, 0.0f};
inline constexpr auto up_v = glm::vec3{0.0f, 1.0f, 0.0f};
inline constexpr auto front_v = glm::vec3{0.0f, 0.0f, 1.0f};
inline constexpr auto forward_v = -front_v;

inline constexpr auto identity_mat_v = glm::identity<glm::mat4>();

inline constexpr auto look_front_v = glm::identity<glm::quat>();

inline auto get_look_forward() -> glm::quat const& {
	static auto const ret = glm::angleAxis(glm::radians(180.0f), up_v);
	return ret;
}

/// \brief 3D Transform with cached matrix.
class Transform {
  public:
	static constexpr auto inspect_control_width_v{200.0f};

	struct Data {
		glm::vec3 position{};
		glm::quat orientation{glm::identity<glm::quat>()};
		glm::vec3 scale{1.0f};

		[[nodiscard]] auto get_matrix() const -> glm::mat4 {
			auto const t = glm::translate(identity_mat_v, position);
			auto const r = glm::toMat4(orientation);
			auto const s = glm::scale(identity_mat_v, scale);
			return t * r * s;
		}
	};

	[[nodiscard]] static auto to_orientation(Radians yaw, Radians pitch) -> glm::quat { return glm::quat{glm::vec3{pitch.value, yaw.value, 0.0f}}; }

	[[nodiscard]] auto get_position() const -> glm::vec3 const& { return m_data.position; }
	auto set_position(glm::vec3 const& position) -> Transform&;

	[[nodiscard]] auto get_orientation() const -> glm::quat const& { return m_data.orientation; }
	auto set_orientation(glm::quat const& orientation) -> Transform&;

	[[nodiscard]] auto get_scale() const -> glm::vec3 const& { return m_data.scale; }
	auto set_scale(glm::vec3 const& scale) -> Transform&;

	[[nodiscard]] auto get_matrix() const -> glm::mat4 const&;
	[[nodiscard]] auto to_matrix(glm::mat4 const& parent) const -> glm::mat4 { return parent * get_matrix(); }
	void from_matrix(glm::mat4 const& matrix);

	[[nodiscard]] auto get_data() const -> Data const& { return m_data; }
	void set_data(Data const& data);

	static auto im_inspect_position(glm::vec3& out_position) -> bool;
	static auto im_inspect_orientation(glm::quat& out_orientation) -> bool;
	static auto im_inspect_scale(glm::vec3& out_scale) -> bool;

	void im_inspect();

	inline static bool im_unified_scale{true};

  private:
	void refresh() const;

	Data m_data{};
	mutable glm::mat4 m_matrix{1.0f};
	mutable bool m_dirty{};
};
} // namespace levk
