#pragma once
#include <le/cli/args.hpp>
#include <le/core/polymorphic.hpp>
#include <le/graphics/rgba.hpp>
#include <cstdint>
#include <string>

namespace le::cli {
class Command : public Polymorphic {
  public:
	enum Flag : std::uint32_t {
		eClearLog = 1 << 0,
	};

	struct Response {
		std::string text{};
		graphics::Rgba rgba{graphics::white_v};
	};

	[[nodiscard]] virtual auto get_name() const -> std::string_view = 0;

	[[nodiscard]] auto get_flags() const -> std::uint32_t { return m_flags; }
	[[nodiscard]] auto get_response() const -> Response const& { return response; }

	void execute(Args args);

  protected:
	virtual void execute() = 0;

	[[nodiscard]] auto has_args() const -> bool { return !m_args.is_empty(); }
	[[nodiscard]] auto next_arg() -> std::string_view { return m_args.next(); }

	void clear_log() { m_flags |= eClearLog; }

	Response response{};

  private:
	Args m_args{};
	std::uint32_t m_flags{};
};
} // namespace le::cli
