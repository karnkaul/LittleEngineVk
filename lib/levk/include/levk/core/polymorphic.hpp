#pragma once

namespace levk {
/// \brief Base class for polymorphic types.
class Polymorphic {
  public:
	Polymorphic(Polymorphic const&) = default;
	Polymorphic(Polymorphic&&) = default;
	auto operator=(Polymorphic const&) -> Polymorphic& = default;
	auto operator=(Polymorphic&&) -> Polymorphic& = default;

	Polymorphic() = default;
	virtual ~Polymorphic() = default;
};

/// \brief Base class for non-copiable and non-movable polymorphic types.
class PolyPinned {
  public:
	PolyPinned(PolyPinned const&) = delete;
	PolyPinned(PolyPinned&&) = delete;
	auto operator=(PolyPinned const&) -> PolyPinned& = delete;
	auto operator=(PolyPinned&&) -> PolyPinned& = delete;

	PolyPinned() = default;
	virtual ~PolyPinned() = default;
};
} // namespace levk
