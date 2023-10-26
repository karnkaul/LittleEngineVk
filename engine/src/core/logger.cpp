#include <le/core/logger.hpp>
#include <array>
#include <atomic>
#include <cassert>
#include <condition_variable>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <mutex>
#include <thread>

#if defined(_WIN32)
#include <Windows.h>
#endif

namespace le {
namespace {
namespace fs = std::filesystem;

auto thread_id() -> int {
	auto const next_id = []() -> int {
		static std::atomic<int> s_id{};
		return s_id++;
	};
	thread_local auto const ret{next_id()};
	return ret;
}

struct Timestamp {
	static constexpr std::size_t capacity_v{128};

	auto operator()(logger::Clock::time_point const& time_point) const -> std::array<char, capacity_v> {
		auto ret = std::array<char, capacity_v>{};
		auto const time = logger::Clock::to_time_t(time_point);
		static auto s_mutex{std::mutex{}};
		auto lock = std::unique_lock{s_mutex};
		// NOLINTNEXTLINE
		auto const tm_ = *std::localtime(&time);
		lock.unlock();
		std::strftime(ret.data(), ret.size(), "%T", &tm_);
		return ret;
	}
};

auto format_line(std::string_view const domain, std::string_view const message, char level) {
	return std::format("[{}][T{}] [{}] {} [{}]\n", level, thread_id(), domain, message, Timestamp{}(logger::Clock::now()).data());
}

// NOLINTNEXTLINE
std::weak_ptr<logger::File> g_file{};
} // namespace

struct logger::File {
	std::mutex mutex{};
	std::string path{};
	std::string buffer{};
	std::condition_variable_any cv{};
	std::jthread thread{};

	File() : thread([this](std::stop_token const& stop) { run(stop); }) {}

	auto set_path(std::string path) -> void {
		auto lock = std::scoped_lock{mutex};
		if (fs::exists(path)) { fs::remove(path); }
		this->path = std::move(path);
	}

	auto push(std::string_view text) -> void {
		auto lock = std::unique_lock{mutex};
		buffer.append(text);
		lock.unlock();
		cv.notify_one();
	}

	auto run(std::stop_token const& stop) -> void {
		while (!stop.stop_requested()) {
			auto lock = std::unique_lock{mutex};
			cv.wait(lock, stop, [this] { return !buffer.empty(); });
			if (buffer.empty()) { continue; }
			if (auto file = std::ofstream{path, std::ios::binary | std::ios::app}) {
				file << buffer;
				buffer.clear();
			}
		}
	}
};

auto logger::print(std::string_view const domain, std::string_view const message, char level) -> void {
	auto const line = format_line(domain, message, level);
	assert(!line.empty() && line.back() == '\n');
	if (auto file = g_file.lock()) { file->push(line); }
	auto& stream = level == error_v ? std::cerr : std::cout;
	stream << line;
#if defined(_WIN32)
	OutputDebugStringA(line.c_str());
#endif
}

auto logger::log_to_file(std::string path) -> std::shared_ptr<File> {
	auto file = g_file.lock();
	if (!file) {
		file = std::make_shared<File>();
		g_file = file;
	}
	file->set_path(std::move(path));
	return file;
}
} // namespace le
