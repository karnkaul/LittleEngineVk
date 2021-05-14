#pragma once
#include <string>
#include <vector>
#include <core/traits.hpp>

namespace le::io {
#if defined(_WIN32) || defined(_WIN64)
inline constexpr char g_separator = '\\';
#else
inline constexpr char g_separator = '/';
#endif

///
/// \brief Lightweight std::filesystem::path alternative
///
class Path {
  public:
	Path() = default;
	Path(std::string_view str);
	template <typename T, typename = require<!std::is_same_v<T, Path>>>
	Path(T const& t);

	bool has_parent_path() const;
	bool empty() const;
	bool has_filename() const;
	bool has_extension() const;
	bool has_root_directory() const;

	Path parent_path() const;
	Path filename() const;
	Path extension() const;

	std::string string() const;
	std::string generic_string() const;
	void clear();

	Path& append(Path const& rhs);
	Path& operator/=(Path const& rhs);
	Path& concat(Path const& rhs);
	Path& operator+=(Path const& rhs);

  protected:
	explicit Path(std::vector<std::string>&& units);
	std::string to_string(char separator) const;

	std::vector<std::string> m_units;
};

Path absolute(Path const& path);
Path current_path();
bool is_regular_file(Path const& path);
bool is_directory(Path const& path);

bool operator==(Path const& lhs, Path const& rhs);
bool operator!=(Path const& lhs, Path const& rhs);

template <typename T, typename>
Path::Path(T const& t) : Path(std::string_view(t)) {}

inline Path operator/(Path const& lhs, Path const& rhs) {
	auto ret = lhs;
	return ret /= rhs;
}

inline Path operator+(Path const& lhs, Path const& rhs) {
	auto ret = lhs;
	return ret += rhs;
}
} // namespace le::io
