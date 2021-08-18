#pragma once
#include <core/services.hpp>
#include <engine/editor/gadget.hpp>
#include <engine/editor/widget.hpp>

namespace le::edi {
class Inspector : public Control, public Service<Inspector> {
  public:
	void update() override;

	template <typename T, typename... Args>
		requires(std::is_base_of_v<Gadget, T> || std::is_base_of_v<GuiGadget, T>)
	T& attach(Args&&... args);
	bool detach(std::string const& id);

  private:
	std::vector<std::unique_ptr<Gadget>> m_gadgets;
	std::vector<std::unique_ptr<GuiGadget>> m_guiGadgets;
};

// impl

template <typename T, typename... Args>
	requires(std::is_base_of_v<Gadget, T> || std::is_base_of_v<GuiGadget, T>)
T& Inspector::attach(Args&&... args) {
	if constexpr (std::is_base_of_v<Gadget, T>) {
		m_gadgets.push_back(std::make_unique<T>(std::forward<Args>(args)...));
		return static_cast<T&>(*m_gadgets.back());
	} else {
		m_guiGadgets.push_back(std::make_unique<T>(std::forward<Args>(args)...));
		return static_cast<T&>(*m_guiGadgets.back());
	}
}
} // namespace le::edi
