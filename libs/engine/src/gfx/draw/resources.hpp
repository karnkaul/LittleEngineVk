#pragma once
#include <type_traits>
#include <unordered_map>
#include "core/map_store.hpp"
#include "resource.hpp"

namespace le::gfx
{
inline class Resources* g_pResources = nullptr;

class Resources final
{
public:
	class Service final
	{
	public:
		Service();
		~Service();
	};

private:
	TMapStore<std::unordered_map<std::string, std::unique_ptr<Resource>>> m_resources;

public:
	Resources();
	~Resources();

	template <typename T, typename = std::enable_if_t<std::is_base_of_v<Resource, T>>>
	T* create(std::string const& id, typename T::Info info)
	{
		auto uT = std::make_unique<T>(std::move(info));
		if (uT)
		{
			uT->setup(id);
			m_resources.insert(id, std::move(uT));
		}
		return get<T>(id);
	}

	template <typename T, typename = std::enable_if_t<std::is_base_of_v<Resource, T>>>
	T* get(std::string const& id)
	{
		auto [pT, bResult] = m_resources.get(id);
		if (bResult && pT)
		{
			return dynamic_cast<T*>(pT->get());
		}
		return nullptr;
	}

	template <typename T, typename = std::enable_if_t<std::is_base_of_v<Resource, T>>>
	bool unload(std::string const& id)
	{
		return m_resources.unload(id);
	}

	void unloadAll();

	void update();
};
} // namespace le::gfx
