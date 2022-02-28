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
	struct Cache;

  protected:
	std::size_t m_nextIndex[max_bindings_v] = {};
};

class DescriptorUpdater : public DescriptorHelper {
  public:
	DescriptorUpdater() = default;
	DescriptorUpdater(not_null<Cache const*> cache, not_null<ShaderInput*> input, not_null<DrawBindings const*> bindings, u32 setNumber, std::size_t index);

	bool valid() const noexcept { return m_input && m_vram && m_bindings; }

	template <typename T>
	bool update(u32 bind, T const& t, vk::DescriptorType type = vk::DescriptorType::eUniformBuffer) const;
	bool update(u32 bind, Opt<Texture const> tex, TextureFallback tb = TextureFallback::eWhite) const;
	bool update(u32 bind, ShaderBuffer const& buffer) const;

  private:
	bool check(u32 bind, vk::DescriptorType const* type = {}, Texture::Type const* texType = {}) const noexcept;
	Texture const& safeTex(Texture const* tex, u32 bind, TextureFallback fb) const;

	mutable ktl::fixed_vector<u32, max_bindings_v> m_binds;
	Cache const* m_cache{};
	ShaderInput* m_input{};
	graphics::VRAM* m_vram{};
	DrawBindings const* m_bindings{};
	std::size_t m_index{};
	u32 m_setNumber{};
};

class DescriptorMap : public DescriptorHelper {
  public:
	explicit DescriptorMap(not_null<Cache const*> cache, not_null<ShaderInput*> input) noexcept : m_input(input), m_cache(cache) {}

	bool contains(u32 setNumber) { return m_input->contains(setNumber); }
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
	bool bindNext(u32 setNumber);
	template <typename... T>
		requires(sizeof...(T) > 1)
	void bindNext(T const... setNumbers) { (bindNext(setNumbers), ...); }

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
		m_input->set(m_setNumber, m_index).writeUpdate(t, bind);
		m_bindings->indices[m_setNumber] = m_index;
		return true;
	}
	return false;
}
} // namespace le
