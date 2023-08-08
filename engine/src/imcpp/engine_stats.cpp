#include <imgui.h>
#include <le/core/fixed_string.hpp>
#include <le/engine.hpp>
#include <le/imcpp/engine_stats.hpp>

namespace le::imcpp {
auto EngineStats::draw_to(OpenWindow /*w*/) -> void {
	auto& engine = Engine::self();
	auto const& stats = engine.get_stats();

	m_frame_times.get_current() = std::chrono::duration<float, std::milli>{stats.frame.time}.count();

	ImGui::Text("%s", FixedString{"GPU: {}", stats.gpu_name}.c_str());
	ImGui::Text("%s", FixedString{"validation layers: {}", stats.validation_enabled ? "on" : "off"}.c_str());

	auto const& renderer = graphics::Renderer::self();
	auto const current_mode = renderer.get_present_mode();
	if (auto combo = Combo{"vsync", graphics::Renderer::to_vsync_string(current_mode).data()}) {
		for (auto const present_mode : renderer.get_supported_present_modes()) {
			if (combo.item(graphics::Renderer::to_vsync_string(present_mode).data(), present_mode == current_mode)) {
				engine.request_present_mode(present_mode);
			}
		}
	}

	auto const framebuffer_extent = engine.framebuffer_extent();
	ImGui::Text("%s", FixedString{"framebuffer: {}x{}", framebuffer_extent.x, framebuffer_extent.y}.c_str());
	auto const window_extent = engine.window_extent();
	ImGui::Text("%s", FixedString{"window: {}x{}", window_extent.x, window_extent.y}.c_str());

	ImGui::Separator();
	if (auto tn = TreeNode{"frame"}) {
		ImGui::Text("%s", FixedString{"count: {}", stats.frame.count}.c_str());
		ImGui::Text("%s", FixedString{"draw calls: {}", stats.frame.draw_calls}.c_str());
		auto const overlay_text = FixedString{"{:.1f}ms", m_frame_times.get_current()};
		ImGui::PlotLines("time", m_frame_times.data.data(), static_cast<int>(m_frame_times.data.size()), static_cast<int>(m_frame_times.offset),
						 overlay_text.c_str(), 0.1f, 30.0f, {0.0f, 50.0f});
		ImGui::Text("%s", FixedString{"rate: {}FPS", stats.frame.rate}.c_str());
	}
	if (auto tn = TreeNode{"cache"}) {
		ImGui::Text("%s", FixedString{"shaders: {}", stats.cache.shaders}.c_str());
		ImGui::Text("%s", FixedString{"pipelines: {}", stats.cache.pipelines}.c_str());
		ImGui::Text("%s", FixedString{"vertex buffers: {}", stats.cache.vertex_buffers}.c_str());
	}

	ImGui::Separator();
	if (auto tn = TreeNode{"Frame Profile"}) {
		auto const frame_profile = engine.frame_profile();
		for (auto type = FrameProfile::Type{}; type < FrameProfile::Type::eCOUNT_; type = FrameProfile::Type(int(type) + 1)) {
			auto const ratio = frame_profile.profile[type] / frame_profile.frame_time;
			auto const label = FrameProfile::to_string_v[type];
			auto const overlay = FixedString{"{} ({:.0f}%)", label, ratio * 100.0f};
			ImGui::ProgressBar(ratio, {-1.0f, 0.0f}, overlay.c_str());
		}
	}

	m_frame_times.advance();
}

auto EngineStats::set_frame_time_count(std::size_t const count) -> void {
	if (count == 0) { return; }
	m_frame_times.data.resize(count);
	if (m_frame_times.offset >= m_frame_times.data.size()) { m_frame_times.offset = m_frame_times.data.size() - 1; }
}
} // namespace le::imcpp
