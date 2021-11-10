#pragma once
#include <core/services.hpp>
#include <engine/editor/gadget.hpp>
#include <engine/editor/types.hpp>

namespace le::edi {
class Inspector {
  public:
	static constexpr std::string_view title_v = "Inspector";

	template <typename T, typename... Args>
		requires(std::is_base_of_v<Gadget, T> || std::is_base_of_v<GuiGadget, T>)
	static T& attach(Args&&... args);
	static bool detach(std::string const& id);

  private:
	static void update(SceneRef const& scene);
	static void clear();

	inline static std::vector<std::unique_ptr<Gadget>> s_gadgets;
	inline static std::vector<std::unique_ptr<GuiGadget>> s_guiGadgets;

	friend class Sudo;
};

// impl

template <typename T, typename... Args>
	requires(std::is_base_of_v<Gadget, T> || std::is_base_of_v<GuiGadget, T>)
T& Inspector::attach(Args&&... args) {
	auto t = std::make_unique<T>(std::forward<Args>(args)...);
	auto ret = t.get();
	if constexpr (std::is_base_of_v<Gadget, T>) {
		s_gadgets.push_back(std::move(t));
	} else {
		s_guiGadgets.push_back(std::move(t));
	}
	return *ret;
}
} // namespace le::edi
