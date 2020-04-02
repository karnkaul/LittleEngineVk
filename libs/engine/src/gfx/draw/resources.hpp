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
	T* create(stdfs::path const& id, typename T::Info info)
	{
		ASSERT(!m_resources.get(id.generic_string()).bResult, "ID already loaded!");
		auto uT = std::make_unique<T>(id, std::move(info));
		T* pT = nullptr;
		if (uT)
		{
			uT->setup();
			pT = uT.get();
			m_resources.insert(id.generic_string(), std::move(uT));
		}
		return pT;
	}

	template <typename T, typename = std::enable_if_t<std::is_base_of_v<Resource, T>>>
	T* get(stdfs::path const& id)
	{
		auto [pT, bResult] = m_resources.get(id.generic_string());
		if (bResult && pT)
		{
			return dynamic_cast<T*>(pT->get());
		}
		return nullptr;
	}

	template <typename T, typename = std::enable_if_t<std::is_base_of_v<Resource, T>>>
	bool unload(stdfs::path const& id)
	{
		return m_resources.unload(id.generic_string());
	}

	void unloadAll();

	void update();

private:
	void init();

	friend class Service;
};
} // namespace le::gfx
