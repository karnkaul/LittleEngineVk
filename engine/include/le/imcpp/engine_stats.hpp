#pragma once
#include <le/core/time.hpp>
#include <le/core/wrap.hpp>
#include <le/imcpp/common.hpp>

namespace le::imcpp {
class EngineStats {
  public:
	static constexpr std::size_t frame_time_count_v{200};

	auto draw_to(OpenWindow w) -> void;

	auto set_frame_time_count(std::size_t count) -> void;

  private:
	auto push_frame_time(Duration dt) -> void;

	Wrap<std::vector<float>> m_frame_times{std::vector<float>(frame_time_count_v)};
};
} // namespace le::imcpp
