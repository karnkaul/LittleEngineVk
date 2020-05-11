#include "engine/ecs/component.hpp"
#include "engine/ecs/registry.hpp"

namespace le::ecs
{
Component::Component() = default;
Component::Component(Component&&) noexcept = default;
Component& Component::operator=(Component&&) noexcept = default;
Component::~Component() = default;

Entity* Component::entity()
{
	return m_pOwner;
}

Entity const* Component::entity() const
{
	return m_pOwner;
}

void Component::onCreate()
{
	return;
}

void Component::create(Entity* pOwner, Sign sign)
{
	m_pOwner = pOwner;
	m_sign = sign;
	onCreate();
	return;
}
} // namespace le::ecs
