#pragma once
#include <le/cli/command.hpp>
#include <le/cli/console.hpp>

namespace le::cli {
class Responder : public Polymorphic {
  public:
	virtual void autocomplete(Console& console) const = 0;
	virtual void submit(Console& console) = 0;
};
} // namespace le::cli
