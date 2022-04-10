#pragma once
#include <ktl/async/kmutex.hpp>
#include <ktl/kunique_ptr.hpp>
#include <levk/core/kassert/kassert.hpp>
#include <levk/core/log.hpp>
#include <stdexcept>
#include <string>
#include <vector>

namespace le {
struct SrcLocPrint {
	enum class Type { eNone, eFilename, ePath, eAbsolute };

	SrcLoc loc{};
	Type type = Type::ePath;

	std::string to_string() const;
};

std::ostream& operator<<(std::ostream& out, SrcLocPrint const& slp);

struct AssertContext {
	inline static SrcLocPrint::Type locPrintType{SrcLocPrint::Type::ePath};

	AssertData<std::string_view> data{};

	// Assertion failed! [<expression>]
	//   <message>
	//   function [filename:line]
	struct Formatter {
		std::string operator()(AssertContext const&) const;
	};

	template <typename Fmt = Formatter>
	struct Printer {
		Fmt fmt{};
		void operator()(AssertContext const& ctx) const { logE(fmt(ctx)); }
	};
};

struct AssertRecord {
	AssertData<std::string> data{};
	int threadID = dlog::this_thread_id();

	AssertRecord() = default;
	AssertRecord(AssertContext const& context) : data{std::string(context.data.expression), std::string(context.data.message), context.data.location} {}
};

struct AssertException : std::runtime_error {
	using runtime_error::runtime_error;
	AssertException(AssertContext const& context) : runtime_error(context.data.message.data()) {}
};

struct AssertHandler {
	virtual ~AssertHandler() = default;

	virtual void notify(AssertContext const& ctx) { AssertContext::Printer<>{}(ctx); }
	virtual void trigger(AssertContext const& ctx) noexcept(false) { throw AssertException(ctx); }
};

struct NullAssertHandler : AssertHandler {
	void notify(AssertContext const&) override {}
	void trigger(AssertContext const&) override {}
};

template <typename Storage = std::vector<AssertRecord>, typename Base = AssertHandler>
struct AssertRecorder : Base {
	ktl::strict_tmutex<Storage> records{};

	void notify(AssertContext const& context) override {
		Base::notify(context);
		auto lock = ktl::klock(records);
		lock->insert(lock->end(), context);
	}
};

class AssertInstance {
  public:
	AssertInstance(ktl::kunique_ptr<AssertHandler>&& handler = ktl::make_unique<AssertHandler>()) { setHandler(std::move(handler)); }
	~AssertInstance();

	AssertHandler const* handler() const;
	void setHandler(ktl::kunique_ptr<AssertHandler>&& handler);
};
} // namespace le
