#pragma once
#include <levk/core/polymorphic.hpp>
#include <functional>
#include <memory>

namespace levk {
/// \brief Queue for deferred destruction of resources.
class IDeferQueue : public Polymorphic {
  public:
	/// \brief Defer an object.
	/// \param t object to defer.
	template <typename Type>
	void push_object(Type t) {
		do_push(std::make_unique<Model<Type>>(std::move(t)));
	}

	void push_callback(std::move_only_function<void()> callback) { do_push(std::move(callback)); }

  protected:
	struct Base : Polymorphic {};

	template <typename Type>
	struct Model : Base {
		Type t;
		explicit Model(Type t) : t(std::move(t)) {}
	};

	virtual void do_push(std::unique_ptr<Base> t) = 0;
	virtual void do_push(std::move_only_function<void()> f) = 0;
};
} // namespace levk
