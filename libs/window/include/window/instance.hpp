#pragma once
#include <cassert>
#include <core/mono_instance.hpp>
#include <window/event_queue.hpp>
#include <window/surface.hpp>

namespace le::window {
template <typename T>
class Instance : public TMonoInstance<T>, public TSurface<T> {
  public:
	enum class Status { eIdle, eRunning, eClosing, eCOUNT_ };
	constexpr static EnumArray<Status> const statusNames = {"Idle", "Running", "Closing"};

	// struct Meta;
	Instance(bool bValid) : TMonoInstance<T>(bValid) {
	}

	Status status() const noexcept {
		return m_status;
	}

	EventQueue pollEvents() {
		return cast().pollEvents();
	}

	bool close() {
		if (m_status == Status::eRunning) {
			cast().close();
			m_status = Status::eIdle;
			return true;
		}
		return false;
	}

  protected:
	Status m_status = Status::eIdle;

  private:
	T& cast() {
		return static_cast<T&>(*this);
	}
};

enum class Style { eDecoratedWindow = 0, eBorderlessWindow, eBorderlessFullscreen, eDedicatedFullscreen, eCOUNT_ };
constexpr static EnumArray<Style> const styleNames = {"Decorated Window", "Borderless Window", "Borderless Fullscreen", "Dedicated Fullscreen"};

struct CreateInfo {
	struct {
		std::string title;
		glm::ivec2 size = {32, 32};
		glm::ivec2 centreOffset = {};
	} config;

	struct {
		Style style = Style::eDecoratedWindow;
		u8 screenID = 0;
		bool bCentreCursor = true;
		bool bAutoShow = false;
	} options;
};

Key parseKey(std::string_view str) noexcept;
Action parseAction(std::string_view str) noexcept;
Mods parseMods(Span<std::string> vec) noexcept;
Axis parseAxis(std::string_view str) noexcept;
} // namespace le::window
