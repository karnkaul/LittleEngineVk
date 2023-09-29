#include <le/cli/command.hpp>

namespace le::cli {
void Command::execute(Args args) {
	m_args = args;
	m_flags = {};
	response = {};
	execute();
}
} // namespace le::cli
