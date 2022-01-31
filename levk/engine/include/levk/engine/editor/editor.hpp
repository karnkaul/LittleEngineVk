#pragma once
#include <levk/engine/engine.hpp>

namespace le {
namespace graphics {
class CommandBuffer;
}

namespace editor {
struct MenuList;
class SceneRef;

class Instance {
  public:
	static Instance make(Engine::Service engine);

	Instance() noexcept;
	Instance(Instance&&) noexcept;
	Instance& operator=(Instance&&) noexcept;
	~Instance() noexcept;

	explicit operator bool() const noexcept;

	struct Impl;

  private:
	Instance(std::unique_ptr<Impl>&& impl) noexcept;
	std::unique_ptr<Impl> m_impl;
};

inline Viewport g_comboView = {{0.2f, 0.0f}, {0.0f, 20.0f}, 0.6f};

bool engaged() noexcept;
void engage(bool set) noexcept;
void toggle() noexcept;

Viewport const& view() noexcept;
MenuList& menu() noexcept;
bool exists() noexcept;
void showImGuiDemo() noexcept;

graphics::ScreenView update(SceneRef scene);
void render(graphics::CommandBuffer const& cb);
} // namespace editor
} // namespace le
