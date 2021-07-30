#pragma once
#include <map>
#include <core/std_types.hpp>
#include <core/utils/vbase.hpp>
#include <ktl/monotonic_map.hpp>

namespace le::input {
struct State;
class Receiver;

using Receivers = ktl::monotonic_map<Receiver*, std::map<u64, Receiver*>>;
using Handle = Receivers::handle;

class Receiver : public utils::VBase {
  public:
	virtual bool block(State const& state) = 0;

	Handle m_inputHandle;
};
} // namespace le::input
