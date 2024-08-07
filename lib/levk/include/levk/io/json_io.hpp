#pragma once
#include <djson/json.hpp>
#include <levk/core/time.hpp>
#include <levk/geometry.hpp>
#include <levk/lights.hpp>
#include <levk/transform.hpp>

namespace levk {
template <dj::Numeric Type, glm::length_t Length>
void to_json(dj::Json& json, glm::vec<Length, Type> const& in) {
	json.push_back(in.x);
	if constexpr (Length > 1) { json.push_back(in.y); }
	if constexpr (Length > 2) { json.push_back(in.z); }
	if constexpr (Length > 3) { json.push_back(in.w); }
}

template <dj::Numeric Type, glm::length_t Length>
void from_json(dj::Json const& in, glm::vec<Length, Type>& out) {
	if (in.is_number()) {
		out = glm::vec<Length, Type>{in.as<Type>()};
	} else {
		out.x = in[0].as<Type>();
		if constexpr (Length > 1) { out.y = in[1].as<Type>(); }
		if constexpr (Length > 2) { out.z = in[2].as<Type>(); }
		if constexpr (Length > 3) { out.w = in[3].as<Type>(); }
	}
}

void to_json(dj::Json& json, glm::quat const& quat);
void from_json(dj::Json const& json, glm::quat& out);

void to_json(dj::Json& json, glm::mat4 const& mat);
void from_json(dj::Json const& json, glm::mat4& out);

void to_json(dj::Json& json, Transform::Data const& transform);
void from_json(dj::Json const& json, Transform::Data& out);

template <dj::Numeric Type>
void to_json(dj::Json& out, Rect<Type> const& in) {
	to_json(out["lt"], in.lt);
	to_json(out["rb"], in.rb);
}

template <dj::Numeric Type>
void from_json(dj::Json const& json, Rect<Type>& out) {
	from_json(json["lt"], out.lt);
	from_json(json["rb"], out.rb);
}

void to_json(dj::Json& out, shape::NineSlice const& nine_slice);
void from_json(dj::Json const& json, shape::NineSlice& out);

inline void to_json(dj::Json& out, Radians const& radians) { out = radians.to_degrees().value; }
inline void from_json(dj::Json const& json, Radians& out) { out = Degrees{json.as<float>()}; }

void to_json(dj::Json& out, DirectionalLight const& light);
void from_json(dj::Json const& json, DirectionalLight& out);

inline void to_json(dj::Json& out, Seconds const& seconds) { out = seconds.count(); }
inline void from_json(dj::Json const& json, Seconds& out) { out = Seconds{json.as<float>()}; }
} // namespace levk
