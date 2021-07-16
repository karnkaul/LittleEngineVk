#pragma once
#include <core/utils/vbase.hpp>

namespace le::edi {
class Control : public utils::VBase {
  public:
	virtual void update() = 0;
};
} // namespace le::edi
