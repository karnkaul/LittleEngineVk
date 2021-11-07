#pragma once
#include <graphics/render/command_buffer.hpp>
#include <graphics/render/descriptor_set.hpp>
#include <graphics/render/pipeline.hpp>
#include <map>

namespace le {
class DescriptorHelper {
  public:
	using ShaderInput = graphics::ShaderInput;
	using Texture = graphics::Texture;
	using ShaderBuffer = graphics::ShaderBuffer;
	using CommandBuffer = graphics::CommandBuffer;
	using Pipeline = graphics::Pipeline;
};

class DescriptorUpdater : public DescriptorHelper {
  public:
	DescriptorUpdater() = default;
	explicit DescriptorUpdater(not_null<ShaderInput*> input, u32 setNumber, std::size_t index) noexcept
		: m_input(input), m_vram(input->m_vram), m_index(index), m_setNumber(setNumber) {}

	bool valid() const noexcept { return m_input && m_vram; }

	template <typename T>
	bool update(u32 bind, T const& t, vk::DescriptorType type = vk::DescriptorType::eUniformBuffer);
	bool update(u32 bind, Texture const& texture) { return check(bind) && m_input->update(texture, m_setNumber, bind, m_index); }
	bool update(u32 bind, ShaderBuffer const& buffer) { return check(bind) && m_input->update(buffer, m_setNumber, bind, m_index); }

  private:
	bool check(u32 bind) noexcept;

	ktl::fixed_vector<u32, 16> m_binds;
	ShaderInput* m_input{};
	graphics::VRAM* m_vram{};
	std::size_t m_index{};
	u32 m_setNumber{};
};

class DescriptorMap : public DescriptorHelper {
  public:
	explicit DescriptorMap(not_null<Pipeline*> pipeline) noexcept : m_input(&pipeline->shaderInput()) {}

	bool contains(u32 setNumber) { return m_input->contains(setNumber); }
	DescriptorUpdater set(u32 setNumber);

  private:
	std::map<u32, std::size_t> m_meta;
	not_null<ShaderInput*> m_input;
};

class DescriptorBinder : public DescriptorHelper {
  public:
	explicit DescriptorBinder(not_null<Pipeline*> pipeline, CommandBuffer cb) noexcept : m_cb(cb), m_pipe(pipeline), m_input(&pipeline->shaderInput()) {}

	void operator()(u32 set);
	void operator()(std::initializer_list<u32> sets);

  private:
	CommandBuffer m_cb;
	not_null<Pipeline*> m_pipe;
	not_null<ShaderInput*> m_input;
	std::map<u32, std::size_t> m_meta;
};

// impl

template <typename T>
bool DescriptorUpdater::update(u32 bind, T const& t, vk::DescriptorType type) {
	if (check(bind)) {
		graphics::Buffer const buf = m_vram->makeBO(t, graphics::Device::bufferUsage(type));
		return m_input->update(buf, m_setNumber, bind, m_index, type);
	}
	return false;
}

inline bool DescriptorUpdater::check(u32 bind) noexcept {
	if (!valid()) { return false; }
	for (u32 const b : m_binds) {
		if (bind == b) { return true; }
	}
	if (m_input->contains(m_setNumber, bind)) {
		m_binds.push_back(bind);
		return true;
	}
	return false;
}

inline DescriptorUpdater DescriptorMap::set(u32 setNumber) {
	return contains(setNumber) ? DescriptorUpdater(m_input, setNumber, m_meta[setNumber]++) : DescriptorUpdater();
}

inline void DescriptorBinder::operator()(u32 set) {
	if (m_input->contains(set)) { m_pipe->bindSet(m_cb, set, m_meta[set]++); }
}

inline void DescriptorBinder::operator()(std::initializer_list<u32> sets) {
	for (u32 const set : sets) { (*this)(set); }
}
} // namespace le
