#pragma once
#include <core/services.hpp>
#include <engine/editor/gadget.hpp>
#include <engine/editor/widget.hpp>

namespace le::edi {
class Inspector : public Control, public Service<Inspector> {
  public:
	void update() override;

	template <typename T, typename... Args>
	T& attach(std::string id, Args&&... args);
	bool detach(std::string const& id);

  private:
	std::unordered_map<std::string, std::unique_ptr<Gadget>> m_gadgets;
};

// impl

template <typename T, typename... Args>
T& Inspector::attach(std::string id, Args&&... args) {
	static_assert(std::is_base_of_v<Gadget, T>, "T must derive from Gadget");
	auto const [it, _] = m_gadgets.emplace(std::move(id), std::make_unique<T>(std::forward<Args>(args)...));
	return static_cast<T&>(*it->second);
}
} // namespace le::edi
