#pragma once
#include <ktl/delegate.hpp>
#include <levk/core/utils/vbase.hpp>

namespace le::input {
struct State;
class Receiver;

using ReceiverStore = ktl::observer_store<Receiver*>;

class Receiver : public utils::VBase {
  public:
	using Store = ReceiverStore;

	virtual bool block(State const& state) = 0;

	bool listening() const noexcept { return m_inputHandle.active(); }

	void attach(ReceiverStore& out) {
		m_inputHandle = out.make_handle();
		m_inputHandle.attach(this);
	}
	void detach() { m_inputHandle = {}; }

  protected:
	ReceiverStore::handle m_inputHandle;
};
} // namespace le::input
