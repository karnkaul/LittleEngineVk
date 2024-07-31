#include <levk/core/inclusive_range.hpp>
#include <levk/core/time.hpp>
#include <levk/io/json_io.hpp>

namespace {
// void to_json(dj::Json& out, levk::Radians const& radians) { out = radians.to_degrees().value; }
// void from_json(dj::Json const& json, levk::Radians& out) { out = levk::Degrees{json.as<float>()}; }

// void to_json(dj::Json& out, levk::Seconds const& seconds) { out = seconds.count(); }
// void from_json(dj::Json const& json, levk::Seconds& out) { out = levk::Seconds{json.as<float>()}; }

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

// void levk::to_json(dj::Json& out, TileSheet const& tile_sheet) {
// 	for (auto const& in_tile : tile_sheet.tiles) {
// 		if (in_tile.id.empty()) { continue; }
// 		auto& out_tile = out.push_back({});
// 		out_tile["id"] = in_tile.id;
// 		to_json(out_tile["image_rect"], in_tile.image_rect);
// 		if (!in_tile.colliders.empty()) {
// 			auto& out_colliders = out_tile["colliders"];
// 			for (auto const& in_collider : in_tile.colliders) { to_json(out_colliders.push_back({}), in_collider); }
// 		}
// 	}
// }

// void levk::from_json(dj::Json const& json, TileSheet& out) {
// 	for (auto const& in_tile : json["tiles"].array_view()) {
// 		auto out_tile = TileSheet::Tile{};
// 		out_tile.id = in_tile["id"].as_string();
// 		if (out_tile.id.empty()) { continue; }
// 		from_json(in_tile["image_rect"], out_tile.image_rect);
// 		for (auto const& in_collider : in_tile["colliders"].array_view()) {
// 			auto out_collider = Rect<int>{};
// 			from_json(in_collider, out_collider);
// 			out_tile.colliders.push_back(out_collider);
// 		}
// 		out.tiles.push_back(std::move(out_tile));
// 	}
// }

// void levk::to_json(dj::Json& out, ParticleConfig const& particle_config) {
// 	using ::to_json;
// 	using levk::to_json;

// 	to_json(out["initial"]["position"], particle_config.initial.position);
// 	to_json(out["initial"]["rotation"], particle_config.initial.rotation.lo);

// 	to_json(out["velocity"]["linear"]["angle"], particle_config.velocity.linear.angle);
// 	to_json(out["velocity"]["linear"]["speed"], particle_config.velocity.linear.speed);
// 	to_json(out["velocity"]["angular"], particle_config.velocity.angular);

// 	to_json(out["lerp"]["tint"], particle_config.lerp.tint);
// 	to_json(out["lerp"]["scale"], particle_config.lerp.scale);

// 	to_json(out["ttl"], particle_config.ttl);
// 	to_json(out["quad_size"], particle_config.quad_size);
// 	to_json(out["count"], particle_config.count);
// 	to_json(out["respawn"], particle_config.respawn);
// }

// void levk::from_json(dj::Json const& json, ParticleConfig& out) {
// 	using ::from_json;
// 	using levk::from_json;

// 	from_json(json["initial"]["position"], out.initial.position);
// 	from_json(json["initial"]["rotation"], out.initial.rotation.lo);

// 	from_json(json["velocity"]["linear"]["angle"], out.velocity.linear.angle);
// 	from_json(json["velocity"]["linear"]["speed"], out.velocity.linear.speed);
// 	from_json(json["velocity"]["angular"], out.velocity.angular);

// 	from_json(json["lerp"]["tint"], out.lerp.tint);
// 	from_json(json["lerp"]["scale"], out.lerp.scale);

// 	from_json(json["ttl"], out.ttl);
// 	from_json(json["quad_size"], out.quad_size);
// 	from_json(json["count"], out.count);
// 	from_json(json["respawn"], out.respawn);
// }
