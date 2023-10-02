#pragma once

namespace le {
class Polymorphic {
  public:
	Polymorphic(Polymorphic const&) = default;
	Polymorphic(Polymorphic&&) = default;
	auto operator=(Polymorphic const&) -> Polymorphic& = default;
	auto operator=(Polymorphic&&) -> Polymorphic& = default;

	Polymorphic() = default;
	virtual ~Polymorphic() = default;
};
} // namespace le
