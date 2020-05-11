#pragma once
#include <string>
#include "core/std_types.hpp"
#include "core/flags.hpp"

namespace le::ecs
{
class Entity final
{
public:
	using ID = u64;

public:
	constexpr static ID s_invalidID = 0;
	static std::string const s_tName;

private:
	ID m_id = s_invalidID;
#if defined(LEVK_DEBUG)
public:
	bool m_bDebugThis = false;
#endif

public:
	constexpr static void nextID(ID* out_pID)
	{
		++(*out_pID);
	}

public:
	inline ID id() const
	{
		return m_id;
	}

private:
	friend class Registry;
};
} // namespace le::ecs
