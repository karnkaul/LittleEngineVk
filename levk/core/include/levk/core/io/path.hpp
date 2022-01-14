#pragma once
#include <string>
#include <vector>

namespace le::io {
#if defined(_WIN32) || defined(_WIN64)
inline constexpr char path_separator = '\\';
#else
inline constexpr char path_separator = '/';
#endif

///
/// \brief Lightweight std::filesystem::path alternative
///
class Path {
  public:
	Path() = default;
	Path(std::string_view str);
	template <std::size_t N>
	Path(char const (&str)[N]) : Path(std::string_view(str)) {}
	Path(char const* szStr) : Path(std::string_view(szStr)) {}
	Path(std::string const& str) : Path(std::string_view(str)) {}

	bool operator==(Path const& rhs) const noexcept;

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
bool remove(Path const& path);

inline Path operator/(Path const& lhs, Path const& rhs) {
	auto ret = lhs;
	return ret /= rhs;
}

inline Path operator+(Path const& lhs, Path const& rhs) {
	auto ret = lhs;
	return ret += rhs;
}
} // namespace le::io
