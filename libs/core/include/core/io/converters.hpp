#pragma once
#include <core/std_types.hpp>
#include <dumb_json/json.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

namespace le::io {
template <typename T, typename U>
struct Converter {
	T operator()(U const&) const;
	U operator()(T const&) const;
};

template <typename T>
struct Stringify : Converter<T, std::string> {
	std::string operator()(T const& t) const;
	T operator()(std::string const& str) const;
};

struct JsonHelper;

template <typename T>
struct Jsonify : Converter<T, dj::json> {};

template <typename T>
struct Jsonify<glm::tvec2<T>>;

template <typename T>
struct Jsonify<glm::tvec3<T>>;

template <>
struct Jsonify<glm::quat>;

template <typename T>
std::string toString(T const& t) {
	return Stringify<T>{}(t);
}

template <typename T>
T fromString(std::string const& str) {
	return Stringify<T>{}(str);
}

template <typename T>
dj::json toJson(T const& t) {
	return Jsonify<T>{}(t);
}

template <typename T>
T fromJson(dj::json const& json) {
	return Jsonify<T>{}(json);
}

template <typename T>
	requires dj::json::is_settable<T>
struct Jsonify<T> {
	dj::json operator()(T const& t) const { return dj::json(t); }
	T operator()(dj::json const& json) const { return json.as<T>(); }
};

template <typename T>
std::ostream& operator<<(std::ostream& out, glm::tvec2<T> const& in) {
	return out << in.x << ", " << in.y;
}

template <typename T>
std::ostream& operator<<(std::ostream& out, glm::tvec3<T> const& in) {
	return out << in.x << ", " << in.y << ", " << in.z;
}

inline std::ostream& operator<<(std::ostream& out, glm::quat const& in) { return out << in.x << ", " << in.y << ", " << in.z << ", " << in.w; }

template <typename T>
std::istream& operator>>(std::istream& in, glm::tvec2<T>& out) {
	char discard;
	return in >> out.x >> discard >> out.y;
}

template <typename T>
std::istream& operator>>(std::istream& in, glm::tvec3<T>& out) {
	char discard;
	return in >> out.x >> discard >> out.y >> discard >> out.z;
}

inline std::istream& operator>>(std::istream& in, glm::quat& out) {
	char discard;
	return in >> out.x >> discard >> out.y >> discard >> out.z >> discard >> out.w;
}

struct JsonHelper {
	template <typename T>
	dj::json to(T const& t) const {
		return Jsonify<T>{}(t);
	}

	template <typename T>
	T to(dj::json const& json) const {
		return Jsonify<T>{}(json);
	}

	template <typename T>
	void set(T& out, dj::json const& json) const {
		out = to<T>(json);
	}

	template <typename T, typename... Ts>
	void insert(dj::json& out, std::string key, T value, Ts... others) const {
		out.insert(std::move(key), to(std::move(value)));
		if constexpr (sizeof...(Ts) > 0) { insert(out, std::move(others)...); }
	}

	template <typename... T>
	dj::json build(T... t) const {
		dj::json ret;
		insert(ret, std::move(t)...);
		return ret;
	}
};

template <typename T>
struct Jsonify<glm::tvec2<T>> : JsonHelper {
	dj::json operator()(glm::tvec2<T> t) const { return this->build("x", t.x, "y", t.y); }
	glm::tvec2<T> operator()(dj::json const& json) const { return {this->to<f32>(json.get("x")), this->to<f32>(json.get("y"))}; }
};

template <typename T>
struct Jsonify<glm::tvec3<T>> : JsonHelper {
	dj::json operator()(glm::tvec3<T> t) const { return this->build("x", t.x, "y", t.y, "z", t.z); }
	glm::tvec3<T> operator()(dj::json const& json) const { return {this->to<f32>(json.get("x")), this->to<f32>(json.get("y")), this->to<f32>(json.get("z"))}; }
};

template <>
struct Jsonify<glm::quat> : JsonHelper {
	dj::json operator()(glm::quat t) const { return this->build("x", t.x, "y", t.y, "z", t.z, "w", t.w); }
	glm::quat operator()(dj::json const& json) const {
		return {to<f32>(json.get("x")), to<f32>(json.get("y")), to<f32>(json.get("z")), to<f32>(json.get("w"))};
	}
};

template <typename T>
std::string Stringify<T>::operator()(T const& t) const {
	std::ostringstream ss;
	ss << t;
	return ss.str();
}

template <typename T>
T Stringify<T>::operator()(std::string const& str) const {
	std::istringstream ss(str);
	T ret{};
	ss >> ret;
	return ret;
}
} // namespace le::io
