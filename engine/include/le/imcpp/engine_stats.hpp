#pragma once
#include <glm/vec3.hpp>
#include <le/core/time.hpp>
#include <le/core/wrap.hpp>
#include <le/imcpp/common.hpp>
#include <vector>

namespace le::imcpp {
class EngineStats {
  public:
	struct FpsRgb {
		std::uint32_t lower_bound{};
		glm::vec3 rgb{};
	};

	static constexpr int frame_time_count_v{200};

	auto draw_to(OpenWindow w) -> void;

	auto set_frame_time_count(std::size_t count) -> void;

	int frame_samples{frame_time_count_v};
	std::vector<FpsRgb> fps_rgb{
		FpsRgb{.lower_bound = 30, .rgb = {1.0f, 0.0f, 0.0f}},
		FpsRgb{.lower_bound = 45, .rgb = {1.0f, 1.0f, 0.0f}},
		FpsRgb{.lower_bound = 60, .rgb = {0.25f, 1.0f, 0.0f}},
	};

  private:
	auto push_frame_time(Duration dt) -> void;

	Wrap<std::vector<float>> m_frame_times{};
};
} // namespace le::imcpp
