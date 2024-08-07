#include <levk/core/inclusive_range.hpp>
#include <levk/io/json_io.hpp>

namespace {
template <typename Type>
void to_json(dj::Json& out, levk::InclusiveRange<Type> const& range) {
	using ::to_json;
	using levk::to_json;
	to_json(out["lo"], range.lo);
	to_json(out["hi"], range.hi);
}

template <typename Type>
void from_json(dj::Json const& json, levk::InclusiveRange<Type>& out) {
	using ::from_json;
	using levk::from_json;
	from_json(json["lo"], out.lo);
	from_json(json["hi"], out.hi);
}
} // namespace

void levk::to_json(dj::Json& json, glm::quat const& quat) {
	json.push_back(quat.x);
	json.push_back(quat.y);
	json.push_back(quat.z);
	json.push_back(quat.w);
}

void levk::from_json(dj::Json const& json, glm::quat& out) {
	from_json(json[0], out.x);
	from_json(json[1], out.y);
	from_json(json[2], out.z);
	from_json(json[3], out.w);
}

void levk::to_json(dj::Json& json, glm::mat4 const& mat) {
	to_json(json.push_back({}), mat[0]);
	to_json(json.push_back({}), mat[1]);
	to_json(json.push_back({}), mat[2]);
	to_json(json.push_back({}), mat[3]);
}

void levk::from_json(dj::Json const& json, glm::mat4& out) {
	from_json(json[0], out[0]);
	from_json(json[1], out[1]);
	from_json(json[2], out[2]);
	from_json(json[3], out[3]);
}

void levk::to_json(dj::Json& json, Transform::Data const& transform) {
	to_json(json["position"], transform.position);
	to_json(json["orientation"], transform.orientation);
	to_json(json["scale"], transform.scale);
}

void levk::from_json(dj::Json const& json, Transform::Data& out) {
	from_json(json["position"], out.position);
	from_json(json["orientation"], out.orientation);
	from_json(json["scale"], out.scale);
}

void levk::to_json(dj::Json& out, shape::NineSlice const& nine_slice) {
	to_json(out["n_left_top"], nine_slice.n_left_top);
	to_json(out["n_right_bottom"], nine_slice.n_right_bottom);
}

void levk::from_json(dj::Json const& json, shape::NineSlice& out) {
	from_json(json["n_left_top"], out.n_left_top);
	from_json(json["n_right_bottom"], out.n_right_bottom);
}

void levk::to_json(dj::Json& out, DirectionalLight const& light) {
	to_json(out["orientation"], light.orientation);
	to_json(out["tint"], light.tint);
	to_json(out["inensity"], light.intensity);
}

void levk::from_json(dj::Json const& json, DirectionalLight& out) {
	from_json(json["orientation"], out.orientation);
	from_json(json["tint"], out.tint);
	from_json(json["intensity"], out.intensity);
}
