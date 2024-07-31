#pragma once
#include <levk/core/time.hpp>
#include <levk/executor.hpp>
#include <levk/imcpp/im_text.hpp>
#include <memory>
#include <string>

struct Loader {
	std::unique_ptr<levk::IExecutor> executor{};
	int workers{};

	std::string asset_type{};
	std::string asset_uri{};
	std::string label{};
	levk::Clock::time_point start{};

	auto update() -> bool {
		if (!executor) { return false; }

		if (label.empty()) { label = std::format("Loading {}##Loading", asset_uri); }
		if (!ImGui::IsPopupOpen(label.c_str())) { ImGui::OpenPopup(label.c_str()); }

		auto const status = executor->update();

		auto ret = false;

		auto const center = ImGui::GetMainViewport()->GetCenter();
		ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, {0.5f, 0.5f});
		ImGui::SetNextWindowSize({300.0f, 100.0f});
		if (ImGui::BeginPopupModal(label.c_str(), nullptr)) {
			auto const status_label = levk::FixedString{"{} / {}", status.completed, status.total};
			levk::im_text("progess: {:.2f}", status.get_progress());
			ImGui::SetNextItemWidth(400.0f);
			ImGui::ProgressBar(status.get_progress(), {-FLT_MIN, 0.0f}, status_label.c_str());

			if (!status.is_busy()) {
				ImGui::CloseCurrentPopup();
				ret = true;
			}

			ImGui::EndPopup();
		}

		return ret;
	}
};
