#pragma once
#include <engine/utils/vbase.hpp>

namespace le::edi {
class Control : public utils::VBase {
  public:
	virtual void update() = 0;
};
} // namespace le::edi
