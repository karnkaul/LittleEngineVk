#include <filesystem>
#include <fstream>
#include <iostream>
#include <span>
#include <sstream>
#include <string_view>

#include <clap/clap.hpp>

namespace stdfs = std::filesystem;

namespace {
enum class result { success = 0, invalid_usage = 10, file_not_found = 20, io_failure, git_failure, cmake_failure, unknown = 100 };

bool isspace(char c) noexcept { return std::isspace(static_cast<unsigned char>(c)); }

template <typename T>
struct tresult {
	T t;
	result res;

	tresult(T&& t) : t(std::move(t)), res(result::success) {}
	tresult(result r) : res(r) {}

	bool success() const noexcept { return res == result::success; }
	explicit operator bool() const noexcept { return success(); }

	T const& operator*() const { return t; }
	T& operator*() { return t; }
	T const* operator->() const { return &t; }
	T* operator->() { return &t; }
};

struct str_view {
	std::string_view str;
	std::size_t first{};
	std::size_t last() const noexcept { return first + str.size(); }
};

struct extractor {
	std::span<char const* const> args;

	struct item : std::string_view {
		using std::string_view::string_view;

		explicit operator bool() const noexcept { return !empty(); }
		operator stdfs::path() const { return stdfs::path(std::string(*this)); }
	};

	item advance() noexcept {
		if (!args.empty()) {
			auto ret = args.front();
			args = args.subspan(1);
			return ret;
		}
		return {};
	}
};

struct params {
	std::string declare;
	stdfs::path git;
	stdfs::path cmake = "CMakeLists.txt";
	bool debug{};

	void print() const {
		std::cout << "\nparams:\n- declare_name\t\t: " << declare << "\n- /path/to/.git\t\t: " << git.generic_string()
				  << "\n- /path/to/cmake_script\t: " << cmake.generic_string() << '\n';
	}

	result check() const {
		std::error_code ec;
		if (!stdfs::is_regular_file(cmake, ec)) {
			std::cout << "\n[" << cmake.generic_string() << "] not found\n";
			return result::file_not_found;
		}
		if (!stdfs::is_directory(git, ec)) {
			std::cout << "\n[" << git.generic_string() << "] not found\n";
			return result::file_not_found;
		}
		return result::success;
	}
};

struct property {
	str_view key;
	str_view value;

	static str_view next_token(std::string_view str, std::size_t first) noexcept {
		while (first < str.size() && isspace(str[first])) { ++first; }
		if (first >= str.size()) { return {{}, str.size()}; }
		std::size_t last = first;
		while (last < str.size() && !isspace(str[last])) { ++last; }
		return {str.substr(first, last - first), first};
	}

	static property make(std::string_view str, std::size_t start) noexcept {
		property ret;
		ret.key = next_token(str, start);
		ret.value = next_token(str, ret.key.last());
		return ret;
	}
};

tresult<std::string> file_contents(stdfs::path const& path) {
	if (auto file = std::ifstream(path)) {
		std::ostringstream str;
		str << file.rdbuf();
		return str.str();
	}
	return result::io_failure;
}

struct fetch_content {
	str_view full;
	str_view declare;
	str_view git_tag;

	static tresult<fetch_content> make(std::string_view str, std::size_t start) noexcept {
		static constexpr std::string_view decl = "FetchContent_Declare(";
		if (auto first = str.find(decl, start); first < str.size()) {
			if (auto last = str.find(')', first); last < str.size()) {
				fetch_content ret;
				ret.full = {str.substr(first, last - first + 1), first};
				ret.declare = property::next_token(str, first + decl.size());
				std::size_t next = ret.declare.last();
				while (next < str.size()) {
					auto prop = property::make(str, next);
					if (prop.key.str.empty()) { break; }
					if (prop.key.str == "GIT_TAG") {
						ret.git_tag = prop.value;
						break;
					}
					next = prop.value.last();
				}
				return ret;
			}
		}
		return result::cmake_failure;
	}

	void print() const {
		std::cout << '\n' << full.str << '\n';
		std::cout << "\n- declare\t: " << declare.str << "\n- git_tag\t: " << git_tag.str << '\n';
	}
};

tresult<std::string> git_commit(stdfs::path const& git_dir) {
	std::string const cmd = "cd " + git_dir.string() + " && git rev-parse --short HEAD > commit_hash.txt";
	if (int const ret = std::system(cmd.data()); ret != 0) { return result::io_failure; }
	auto const path = git_dir / "commit_hash.txt";
	auto file = std::ifstream(path);
	if (!file) { return result::io_failure; }
	std::string ret;
	file >> ret;
	file.close();
	std::error_code ec;
	stdfs::remove(path, ec);
	return ret;
}

struct file_commit {
	std::string text;
	std::string commit;

	static tresult<file_commit> make(params const& p) {
		auto t = file_contents(p.cmake);
		if (!t || t->empty()) { return t.res; }
		auto c = git_commit(p.git);
		if (!c || c->empty()) { return c.res; }
		return file_commit{std::move(*t), std::move(*c)};
	}
};

tresult<fetch_content> get_target(std::string_view text, params const& p) {
	for (std::size_t start{}; start < text.size();) {
		auto const ret = fetch_content::make(text, start);
		if (!ret) { return ret; }
		if (p.debug) { ret->print(); }
		if (ret->declare.str == p.declare) { return ret; }
		start = ret->full.last();
	}
	return result::cmake_failure;
}

result write(std::string const& text, stdfs::path const& path) {
	if (auto file = std::ofstream(path)) {
		file << text;
		return result::success;
	}
	return result::io_failure;
}

int ret_val(result res) noexcept { return static_cast<int>(res); }
} // namespace

namespace {
struct u_parser : clap::option_parser {
	params& p;

	u_parser(params& p) : p(p) {
		using namespace clap;
		spec.arg_doc = "DECLARE_NAME PATH_TO_.GIT";
		static constexpr option opts[] = {
			{'v', "verbose", "Verbose mode"},
			{'d', "debug", "debug mode"},
			{'c', "cmake-script", "path to cmake script to update", "PATH0"},
		};
		spec.options = opts;
	}

	bool operator()(clap::option_key key, clap::str_t arg, clap::parse_state state) override {
		switch (key) {
		case 'd': p.debug = true; return true;
		case 'c': p.cmake = arg; return true;
		case no_arg: {
			if (state.arg_index() == 0) {
				p.declare = arg;
				return true;
			} else if (state.arg_index() == 1) {
				p.git = arg;
				return true;
			}
			return false;
		}
		case no_end: return state.arg_index() >= 2;
		default: return false;
		}
	}
};
} // namespace

// Usage: <declare_name> <.git> [script_name]
int main(int argc, char const* const argv[]) {
	params pr;
	u_parser parser(pr);
	clap::program_spec spec;
	spec.version = "0.1";
	spec.parser = &parser;
	spec.doc = "CMake FetchContent commit hash updater";
	auto const ret = clap::parse_args(spec, argc, argv);
	if (ret != clap::parse_result::run) { return 10; }
	if (pr.debug) { pr.print(); }
	auto fc = file_commit::make(pr);
	if (!fc) { return ret_val(fc.res); }
	tresult<fetch_content> const target = get_target(fc->text, pr);
	if (!target) {
		std::cout << "\n[" << pr.cmake.generic_string() << "] failed to find FetchContent_Declare(" << pr.declare << ")\n";
		return ret_val(target.res);
	}
	if (pr.debug) { std::cout << "- git_commit\t: " << fc->commit << "\n\n"; }
	if (target->git_tag.str == fc->commit) {
		std::cout << "[" << pr.declare << "] no changes [" << target->git_tag.str << "] == [" << fc->commit << "]\n";
		return ret_val(result::success);
	}
	std::string const old_commit(target->git_tag.str);
	for (std::size_t i = 0; i < fc->commit.size(); ++i) { fc->text[target->git_tag.first + i] = fc->commit[i]; }
	if (auto res = write(fc->text, pr.cmake); res != result::success) { return ret_val(res); }
	std::cout << "[" << pr.declare << "] updated [" << old_commit << "] to [" << fc->commit << "]\n";
	return ret_val(result::success);
}
