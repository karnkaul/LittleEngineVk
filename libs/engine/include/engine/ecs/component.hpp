#pragma once
#include "core/std_types.hpp"
#include "core/time.hpp"
#include "entity.hpp"

namespace le
{
class Component
{
public:
	using Sign = size_t;

private:
	Sign m_sign = 0;
	Entity m_owner;

public:
	Component();
	Component(Component&&) noexcept;
	Component& operator=(Component&&) noexcept;
	virtual ~Component();

protected:
	virtual void onCreate();

private:
	void create(Entity owner, Sign sign);

private:
	friend class Registry;
};
} // namespace le
