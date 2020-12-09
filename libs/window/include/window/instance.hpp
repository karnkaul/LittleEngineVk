#pragma once
#include <cassert>
#include <core/lib_logger.hpp>
#include <core/mono_instance.hpp>
#include <window/event_queue.hpp>
#include <window/surface.hpp>

namespace le::window {
class IInstance : public TMonoInstance<IInstance>, public ISurface {
  public:
	IInstance(bool bValid) : TMonoInstance(bValid) {
	}
	IInstance(IInstance&&) = default;
	IInstance& operator=(IInstance&&) = default;
	virtual ~IInstance() = default;

	virtual EventQueue pollEvents() = 0;

  protected:
	LibLogger m_log;
};

Key parseKey(std::string_view str) noexcept;
Action parseAction(std::string_view str) noexcept;
Mods parseMods(Span<std::string> vec) noexcept;
Axis parseAxis(std::string_view str) noexcept;
} // namespace le::window
