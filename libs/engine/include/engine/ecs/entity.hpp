#pragma once
#include "core/std_types.hpp"

namespace le
{
struct Entity final
{
	using ID = u64;

	ID id = 0;
};

inline bool operator==(Entity lhs, Entity rhs)
{
	return lhs.id == rhs.id;
}

inline bool operator!=(Entity lhs, Entity rhs)
{
	return !(lhs == rhs);
}
} // namespace le
