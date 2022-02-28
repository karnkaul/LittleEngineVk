#pragma once
#include <levk/graphics/command_buffer.hpp>
#include <levk/graphics/render/descriptor_set.hpp>
#include <levk/graphics/render/draw_list.hpp>
#include <map>

namespace le {
static constexpr auto max_bindings_v = graphics::max_bindings_v;
class AssetStore;

enum class TextureFallback { eWhite, eBlack, eMagenta, eCOUNT_ };

class DescriptorHelper {
  public:
	using ShaderInput = graphics::ShaderInput;
	using Texture = graphics::Texture;
	using ShaderBuffer = graphics::ShaderBuffer;
	using CommandBuffer = graphics::CommandBuffer;
	using DrawBindings = graphics::DrawBindings;
	using DescriptorSet = graphics::DescriptorSet;
	struct Cache;

  protected:
	std::size_t m_nextIndex[max_bindings_v] = {};
};

class DescriptorUpdater : public DescriptorHelper {
  public:
	DescriptorUpdater() = default;
	DescriptorUpdater(not_null<Cache const*> cache, not_null<DescriptorSet*> descriptorSet);

	bool valid() const noexcept { return m_cache && m_descriptorSet; }
	Opt<DescriptorSet> descriptorSet() const noexcept { return m_descriptorSet; }

	template <typename T>
	bool update(u32 binding, T const& t, vk::DescriptorType type = vk::DescriptorType::eUniformBuffer) const;
	bool update(u32 binding, Opt<Texture const> tex, TextureFallback tb = TextureFallback::eWhite) const;
	bool update(u32 binding, ShaderBuffer const& buffer) const;

  private:
	bool check(u32 binding, vk::DescriptorType const* type = {}, Texture::Type const* texType = {}) const noexcept;
	Texture const& safeTex(Texture const* tex, u32 bind, TextureFallback fb) const;

	mutable ktl::fixed_vector<u32, max_bindings_v> m_binds;
	Cache const* m_cache{};
	DescriptorSet* m_descriptorSet{};
};

class DescriptorMap : public DescriptorHelper {
  public:
	explicit DescriptorMap(not_null<Cache const*> cache, not_null<ShaderInput*> input) noexcept : m_input(input), m_cache(cache) {}

	bool contains(u32 setNumber);
	DescriptorUpdater nextSet(DrawBindings const& bindings, u32 setNumber);

	ShaderInput const& shaderInput() const noexcept { return *m_input; }
	Cache const& cache() const noexcept { return *m_cache; }

  private:
	not_null<ShaderInput*> m_input;
	not_null<Cache const*> m_cache;
};

class DescriptorBinder : public DescriptorHelper {
  public:
	explicit DescriptorBinder(vk::PipelineLayout layout, not_null<ShaderInput*> input, CommandBuffer cb) noexcept
		: m_cb(cb), m_input(input), m_layout(layout) {}

	void bind(DrawBindings const& indices) const;

	CommandBuffer const& commandBuffer() const noexcept { return m_cb; }
	vk::PipelineLayout pipelineLayout() const noexcept { return m_layout; }
	ShaderInput const& shaderInput() const noexcept { return *m_input; }

  private:
	CommandBuffer m_cb;
	not_null<ShaderInput*> m_input;
	vk::PipelineLayout m_layout;
};

struct DescriptorHelper::Cache {
	EnumArray<TextureFallback, not_null<Texture const*>> defaults;
	not_null<Texture const*> cube;

	static Cache make(not_null<AssetStore const*> store);
};

// impl

template <typename T>
bool DescriptorUpdater::update(u32 bind, T const& t, vk::DescriptorType type) const {
	if (check(bind, &type)) {
		m_descriptorSet->writeUpdate(t, bind);
		return true;
	}
	return false;
}
} // namespace le
