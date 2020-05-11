#pragma once
#include "core/std_types.hpp"
#include "core/time.hpp"
#include "core/zero.hpp"

namespace le::ecs
{
class Entity;

class Component
{
public:
	using Sign = size_t;

private:
	Sign m_sign = 0;
	Entity* m_pOwner = nullptr;

public:
	Component();
	Component(Component&&) noexcept;
	Component& operator=(Component&&) noexcept;
	virtual ~Component();

public:
	Entity* entity();
	Entity const* entity() const;

protected:
	virtual void onCreate();

private:
	void create(Entity* pOwner, Sign sign);

private:
	friend class Registry;
};

template <typename T>
class Data : public Component
{
public:
	T data;
};
} // namespace le::ecs
