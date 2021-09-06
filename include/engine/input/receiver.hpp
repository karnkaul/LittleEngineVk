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
	virtual void pushSelf(Receivers& out) { m_inputHandle = out.push(this); }
	virtual void popSelf() noexcept { m_inputHandle = {}; }

	bool listening() const noexcept { return m_inputHandle.valid(); }

	Handle m_inputHandle;
};
} // namespace le::input
