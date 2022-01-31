#pragma once
#include <levk/core/utils/expect.hpp>
#include <levk/engine/engine.hpp>
#include <levk/graphics/render/render_pass.hpp>
#include <levk/graphics/render/rgba.hpp>

namespace le {
using RenderPass = graphics::RenderPass;

namespace editor {
class SceneRef;
}

class RenderFrame {
  public:
	RenderFrame(Engine::Service engine, graphics::RenderBegin const& rb = {});
	~RenderFrame();

	explicit operator bool() const noexcept { return valid(); }
	bool valid() const noexcept { return m_renderPass.has_value(); }

	RenderPass& renderPass();
	RenderPass const& renderPass() const;

  private:
	Engine::Service m_engine;
	std::optional<RenderPass> m_renderPass;
};

// impl

inline RenderPass& RenderFrame::renderPass() {
	EXPECT(m_renderPass);
	return *m_renderPass;
}

inline RenderPass const& RenderFrame::renderPass() const {
	EXPECT(m_renderPass);
	return *m_renderPass;
}
} // namespace le
