#pragma once
#include <graphics/command_buffer.hpp>
#include <graphics/render/descriptor_set.hpp>
#include <map>

namespace le {
static constexpr auto max_bindings_v = graphics::max_bindings_v;

class DescriptorHelper {
  public:
	using ShaderInput = graphics::ShaderInput;
	using Texture = graphics::Texture;
	using ShaderBuffer = graphics::ShaderBuffer;
	using CommandBuffer = graphics::CommandBuffer;
};

class DescriptorUpdater : public DescriptorHelper {
  public:
	DescriptorUpdater() = default;
	explicit DescriptorUpdater(not_null<ShaderInput*> input, u32 setNumber, std::size_t index) noexcept
		: m_input(input), m_vram(input->m_vram), m_index(index), m_setNumber(setNumber) {}

	bool valid() const noexcept { return m_input && m_vram; }

	template <typename T>
	bool update(u32 bind, T const& t, vk::DescriptorType type = vk::DescriptorType::eUniformBuffer);
	bool update(u32 bind, Texture const& texture);
	bool update(u32 bind, ShaderBuffer const& buffer);

  private:
	bool check(u32 bind, vk::DescriptorType const* type = {}) noexcept;

	ktl::fixed_vector<u32, max_bindings_v> m_binds;
	ShaderInput* m_input{};
	graphics::VRAM* m_vram{};
	std::size_t m_index{};
	u32 m_setNumber{};
};

class DescriptorMap : public DescriptorHelper {
  public:
	explicit DescriptorMap(not_null<ShaderInput*> input) noexcept : m_input(input) {}

	bool contains(u32 setNumber) { return m_input->contains(setNumber); }
	DescriptorUpdater set(u32 setNumber);

  private:
	u32 m_meta[max_bindings_v] = {};
	not_null<ShaderInput*> m_input;
};

class DescriptorBinder : public DescriptorHelper {
  public:
	explicit DescriptorBinder(vk::PipelineLayout layout, not_null<ShaderInput*> input, CommandBuffer cb) noexcept
		: m_cb(cb), m_layout(layout), m_input(input) {}

	void operator()(u32 set);
	template <typename... T>
		requires(sizeof...(T) > 1)
	void operator()(T const... sets) { ((*this)(sets), ...); }

  private:
	CommandBuffer m_cb;
	vk::PipelineLayout m_layout;
	not_null<ShaderInput*> m_input;
	u32 m_meta[max_bindings_v] = {};
};

// impl

template <typename T>
bool DescriptorUpdater::update(u32 bind, T const& t, vk::DescriptorType type) {
	if (check(bind, &type)) {
		m_input->set(m_setNumber, m_index).writeUpdate(t, bind);
		return true;
	}
	return false;
}

inline bool DescriptorUpdater::update(u32 bind, ShaderBuffer const& buffer) {
	if (check(bind)) {
		m_input->update(buffer, m_setNumber, bind, m_index);
		return true;
	}
	return false;
}

inline bool DescriptorUpdater::update(u32 bind, Texture const& texture) {
	if (check(bind)) {
		m_input->set(m_setNumber, m_index).update(bind, texture);
		return true;
	}
	return false;
}

inline bool DescriptorUpdater::check(u32 bind, vk::DescriptorType const* type) noexcept {
	if (!valid()) { return false; }
	for (u32 const b : m_binds) {
		if (bind == b) { return true; }
	}
	if (m_input->contains(m_setNumber, bind, type)) {
		m_binds.push_back(bind);
		return true;
	}
	return false;
}

inline DescriptorUpdater DescriptorMap::set(u32 setNumber) {
	return contains(setNumber) ? DescriptorUpdater(m_input, setNumber, m_meta[setNumber]++) : DescriptorUpdater();
}

inline void DescriptorBinder::operator()(u32 set) {
	if (m_input->contains(set)) {
		auto const& s = m_input->set(set, m_meta[set]++);
		m_cb.bindSets(m_layout, s.get(), s.setNumber());
	}
}
} // namespace le
