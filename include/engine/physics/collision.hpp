#pragma once
#include <optional>
#include <vector>
#include <core/delegate.hpp>
#include <core/ensure.hpp>
#include <core/not_null.hpp>
#include <core/std_types.hpp>
#include <core/utils/enumerate.hpp>
#include <engine/scene/drawable.hpp>
#include <glm/vec3.hpp>

namespace le {
class Collision {
  public:
	using ID = u64;
	using CFlags = u32;

	static constexpr CFlags all_flags = 0xffffffff;

	struct Rect {
		glm::vec3 scale = glm::vec3(1.0f);
		glm::vec3 offset = {};
	};

	class Collider;
	using OnCollide = Delegate<Collider>;

	class Collider {
	  public:
		Rect& rect() const noexcept { return *m_rect; }
		glm::vec3& position() const noexcept { return *m_pos; }
		[[nodiscard]] OnCollide::Tk onCollide(OnCollide::Callback const& cb) { return m_onCollide->subscribe(cb); }
		CFlags channels() const noexcept { return *m_cflags; }
		void channels(u32 set, u32 unset = 0) const noexcept;

		ID const m_id{};

	  private:
		Collider(ID id, not_null<Rect*> rect, not_null<glm::vec3*> pos, not_null<CFlags*> cflags, not_null<OnCollide*> onCollide) noexcept
			: m_id(id), m_rect(rect), m_pos(pos), m_cflags(cflags), m_onCollide(onCollide) {}

		not_null<Rect*> m_rect;
		not_null<glm::vec3*> m_pos;
		not_null<CFlags*> m_cflags;
		not_null<OnCollide*> m_onCollide;

		friend class Collision;
	};

	Collider add(Rect rect, glm::vec3 position = {}, CFlags chFlags = all_flags);
	bool remove(ID id) noexcept { return m_data.erase(id); }
	void clear() noexcept { m_data.clear(); }

	std::optional<Collider> find(ID id) noexcept;
	void update();

	std::vector<Drawable> drawables() const;

	bool m_issueDrawables = levk_debug;

  private:
	struct Data {
		std::vector<ID> ids;
		std::vector<Rect> rects;
		std::vector<glm::vec3> positions;
		std::vector<CFlags> cflags;
		std::vector<OnCollide> onCollides;
		std::vector<Prop> props;

		void push_back(ID id, Rect rect, glm::vec3 pos, CFlags flags);
		bool erase(ID id) noexcept;
		void clear() noexcept;
	};

	Collider cube(std::size_t i) noexcept;
	void update(std::size_t i, std::size_t j) noexcept;

	Data m_data;
	ID m_nextID{};
};
} // namespace le