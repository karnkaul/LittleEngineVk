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
	template <typename Service, typename... Args>
	void add(Args&&... args);
};

template <typename S>
template <typename... Args>
Services::Model<S>::Model(Args&&... args) : s(std::forward<Args>(args)...)
{
#if defined(LEVK_DEBUG)
	targetType = utils::tName(s);
#endif
}

template <typename Service, typename... Args>
void Services::add(Args&&... args)
{
	m_services.push_back(std::make_unique<Model<Service>>(std::forward<Args>(args)...));
}
} // namespace le
