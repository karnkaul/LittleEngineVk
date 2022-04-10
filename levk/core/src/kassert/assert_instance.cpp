#include <levk/core/kassert/assert_instance.hpp>
#include <filesystem>
#include <sstream>

namespace le {
namespace {
auto g_default = AssertHandler{};
auto g_instanced = ktl::kunique_ptr<AssertHandler>{};

AssertHandler& getHandler() { return g_instanced ? *g_instanced : g_default; }
} // namespace

std::ostream& operator<<(std::ostream& out, SrcLocPrint const& slp) {
	if (slp.type == SrcLocPrint::Type::eNone) { return out; }
	auto path = std::filesystem::path(slp.loc.file_name());
	switch (slp.type) {
	case SrcLocPrint::Type::eFilename: path = path.filename(); break;
	case SrcLocPrint::Type::eAbsolute: path = std::filesystem::absolute(std::move(path)); break;
	default: break;
	}
	out << "[" << path.generic_string() << ':' << slp.loc.line() << ']';
	return out;
}

std::string SrcLocPrint::to_string() const {
	auto str = std::stringstream{};
	str << *this;
	return str.str();
}

std::string AssertContext::Formatter::operator()(AssertContext const& ctx) const {
	auto const& loc = ctx.data.location;
	auto str = std::stringstream{};
	str << "Assertion failed! ";
	if (!ctx.data.expression.empty()) { str << '[' << ctx.data.expression << "] "; }
	str << ctx.data.message << "\n\t" << loc.function_name();
	if (locPrintType != SrcLocPrint::Type::eNone) { str << ' ' << SrcLocPrint{ctx.data.location, locPrintType}; }
	str << '\n';
	return str.str();
}

AssertInstance::~AssertInstance() { g_instanced.reset(); }

AssertHandler const* AssertInstance::handler() const { return g_instanced.get(); }
void AssertInstance::setHandler(ktl::kunique_ptr<AssertHandler>&& handler) { g_instanced = std::move(handler); }
} // namespace le

void le::kassertNotify(AssertData<> const& data) { getHandler().notify({data}); }
void le::kassertTrigger(AssertData<> const& data) noexcept(false) { getHandler().trigger({data}); }
