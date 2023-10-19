#pragma once
#include <le/core/signal.hpp>
#include <le/scene/ui/interactable.hpp>
#include <le/scene/ui/quad.hpp>
#include <le/scene/ui/text.hpp>
#include <le/scene/ui/view.hpp>

namespace le::ui {
class Button : public View, public Interactable {
  public:
	using OnTrigger = Signal<>;

	explicit Button(Ptr<View> parent_view = {});

	auto on_trigger(OnTrigger::Callback callback) { return m_on_trigger.connect(std::move(callback)); }

	template <typename PointerT, typename MemberFuncT>
		requires(std::is_invocable_v<MemberFuncT, PointerT>)
	auto on_trigger(PointerT object, MemberFuncT member_func) {
		return m_on_trigger.connect(object, member_func);
	}

	[[nodiscard]] auto get_quad() const -> Quad& { return *m_quad; }
	[[nodiscard]] auto get_text() const -> Text& { return *m_text; }

	graphics::Rgba no_focus{graphics::white_v};
	graphics::Rgba in_focus{graphics::yellow_v};

  protected:
	void tick(Duration dt) override;

	void on_focus_gained() override { m_quad->set_tint(in_focus); }
	void on_focus_lost() override { m_quad->set_tint(no_focus); }
	void on_release() override { m_on_trigger(); }

  private:
	Ptr<Quad> m_quad{};
	Ptr<Text> m_text{};
	OnTrigger m_on_trigger{};
};
} // namespace le::ui
