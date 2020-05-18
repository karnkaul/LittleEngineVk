#include "engine/ecs/component.hpp"

namespace le
{
Component::Component() = default;
Component::Component(Component&&) noexcept = default;
Component& Component::operator=(Component&&) noexcept = default;
Component::~Component() = default;

void Component::onCreate()
{
	return;
}

void Component::create(Entity owner, Sign sign)
{
	m_owner = owner;
	m_sign = sign;
	onCreate();
	return;
}
} // namespace le
