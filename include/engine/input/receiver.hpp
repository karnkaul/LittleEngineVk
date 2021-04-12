#pragma once
#include <core/tagged_store.hpp>
#include <engine/ibase.hpp>

namespace le::input {
struct State;

using tag_t = Tag<>;

class Receiver : public IBase {
  public:
	virtual bool block(State const& state) = 0;

	tag_t m_inputTag;
};
} // namespace le::input
