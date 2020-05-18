#pragma once
#include <filesystem>
#include <type_traits>
#include "engine/assets/resources.hpp"
#include "component.hpp"

namespace le
{
namespace stdfs = std::filesystem;

template <typename T>
class CData : public Component
{
public:
	T data;
};

template <typename T>
class CResource : public Component
{
	static_assert(std::is_base_of_v<Asset, T>, "T must derive from Asset!");

public:
	stdfs::path m_id;

public:
	CResource(stdfs::path id) : m_id(std::move(id)) {}

public:
	T* get()
	{
		return dynamic_cast<T*>(Resources::inst().get<T>(m_id));
	}

	T const* get() const
	{
		return dynamic_cast<T*>(Resources::inst().get<T>(m_id));
	}
};
} // namespace le
