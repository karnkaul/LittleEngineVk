#pragma once
#include <engine/editor/types.hpp>
#include <typeinfo>

namespace le::edi {
class MainMenu {
  public:
	MainMenu();

	static void showAssetIndexPane();
	template <typename T>
	static std::string_view selectAssetURI(std::string_view filter = {}) {
		return selectAssetURI(typeid(T).hash_code(), filter);
	}

	void operator()(MenuList const& extras) const;

  private:
	static std::string_view selectAssetURI(std::size_t typeHash, std::string_view filter);

	MenuList m_main;
};
} // namespace le::edi
