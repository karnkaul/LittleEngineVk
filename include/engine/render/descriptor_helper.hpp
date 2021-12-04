#pragma once
#include <graphics/command_buffer.hpp>
#include <graphics/render/descriptor_set.hpp>
#include <map>

namespace le {
static constexpr auto max_bindings_v = graphics::max_bindings_v;
class AssetStore;

enum class TexBind { eWhite, eBlack, eMagenta, eCOUNT_ };

class DescriptorHelper {
  public:
	using ShaderInput = graphics::ShaderInput;
	using Texture = graphics::Texture;
	using ShaderBuffer = graphics::ShaderBuffer;
	using CommandBuffer = graphics::CommandBuffer;

	struct Cache;
};

class DescriptorUpdater : public DescriptorHelper {
  public:
	DescriptorUpdater() = default;
	explicit DescriptorUpdater(not_null<Cache const*> cache, not_null<ShaderInput*> input, u32 setNumber, std::size_t index);

	bool valid() const noexcept { return m_input && m_vram; }

	template <typename T>
	bool update(u32 bind, T const& t, vk::DescriptorType type = vk::DescriptorType::eUniformBuffer);
	bool update(u32 bind, Texture const& tex);
	bool update(u32 bind, Texture const* tex, TexBind tb);
	bool update(u32 bind, ShaderBuffer const& buffer);

  private:
	bool check(u32 bind, vk::DescriptorType const* type = {}) noexcept;

	ktl::fixed_vector<u32, max_bindings_v> m_binds;
	Cache const* m_cache{};
	ShaderInput* m_input{};
	graphics::VRAM* m_vram{};
	std::size_t m_index{};
	u32 m_setNumber{};
};

class DescriptorMap : public DescriptorHelper {
  public:
	explicit DescriptorMap(not_null<Cache const*> cache, not_null<ShaderInput*> input) noexcept : m_cache(cache), m_input(input) {}

	bool contains(u32 setNumber) { return m_input->contains(setNumber); }
	DescriptorUpdater set(u32 setNumber);

  private:
	u32 m_meta[max_bindings_v] = {};
	not_null<Cache const*> m_cache;
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

struct DescriptorHelper::Cache {
	EnumArray<TexBind, Texture const*> defaults;

	static Cache make(not_null<AssetStore const*> store);
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
} // namespace le
