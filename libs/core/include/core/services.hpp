#pragma once
#include <filesystem>
#include <memory>
#include <tuple>
#include <vector>
#if defined(LEVK_DEBUG)
#include <string>
#include "utils.hpp"
#endif

namespace le
{
class Services final
{
private:
	struct Concept
	{
#if defined(LEVK_DEBUG)
		std::string targetType;
#endif
		virtual ~Concept() {}
	};
	template <typename S>
	struct Model : public Concept
	{
		S s;

		template <typename... Args>
		Model(Args&&... args);
	};

	std::vector<std::unique_ptr<Concept>> m_services;

public:
	Services();
	Services(Services&&);
	Services& operator=(Services&&);
	~Services();

public:
	template <typename T, typename... Args>
	T& add(Args&&... args);

	template <typename T>
	T* locate();

	template <typename T>
	T const* locate() const;
};

template <typename S>
template <typename... Args>
Services::Model<S>::Model(Args&&... args) : s(std::forward<Args>(args)...)
{
#if defined(LEVK_DEBUG)
	targetType = utils::tName(s);
#endif
}

template <typename T, typename... Args>
T& Services::add(Args&&... args)
{
	auto uT = std::make_unique<Model<T>>(std::forward<Args>(args)...);
	auto& ret = uT->s;
	m_services.push_back(std::move(uT));
	return ret;
}

template <typename T>
T* Services::locate()
{
	for (auto& uConcept : m_services)
	{
		if (auto pModel = dynamic_cast<Model<T>*>(uConcept.get()); pModel)
		{
			return &pModel->s;
		}
	}
	return nullptr;
}

template <typename T>
T const* Services::locate() const
{
	for (auto& uConcept : m_services)
	{
		if (auto pModel = dynamic_cast<Model<T>*>(uConcept.get()); pModel)
		{
			return &pModel->s;
		}
	}
	return nullptr;
}
} // namespace le
