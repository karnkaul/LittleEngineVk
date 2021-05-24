#pragma once
#include <map>
#include <core/std_types.hpp>
#include <engine/ibase.hpp>
#include <kt/monotonic_map/monotonic_map.hpp>

namespace le::input {
struct State;
class Receiver;

using Receivers = kt::monotonic_map<Receiver*, std::map<u64, Receiver*>>;
using Handle = Receivers::handle;

class Receiver : public IBase {
  public:
	virtual bool block(State const& state) = 0;

	Handle m_inputHandle;
};
} // namespace le::input
