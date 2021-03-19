#pragma once
#include <engine/editor/gadget.hpp>
#include <engine/editor/widget.hpp>

namespace le::edi {
class Inspector : public Control {
  public:
	void update() override;

	template <typename T, typename... Args>
	static T& attach(std::string id, Args&&... args);
	static bool detach(std::string const& id);

  private:
	inline static std::unordered_map<std::string, std::unique_ptr<Gadget>> s_gadgets;
};

// impl

template <typename T, typename... Args>
T& Inspector::attach(std::string id, Args&&... args) {
	static_assert(std::is_base_of_v<Gadget, T>, "T must derive from Gadget");
	auto const [it, _] = s_gadgets.emplace(std::move(id), std::make_unique<T>(std::forward<Args>(args)...));
	return static_cast<T&>(*it->second);
}
} // namespace le::edi
