#pragma once
#include <engine/ibase.hpp>

namespace le::edi {
class Control : public IBase {
  public:
	virtual void update() = 0;
};
} // namespace le::edi
