#pragma once
#include <levk/graphics/memory.hpp>
#include <levk/graphics/texture.hpp>
#include <levk/graphics/utils/trotator.hpp>

namespace le::graphics {
class ShaderBuffer;

static constexpr std::size_t max_bindings_v = 16;

///
/// \brief Ring buffered wrapper of a span of vk::DescriptorSets corresponding to a particular set layout
///
class DescriptorSet {
  public:
	struct Img {
		vk::ImageView image;
		vk::Sampler sampler;
	};
	struct Buf {
		vk::Buffer buffer;
		std::size_t size = 0;
	};
	struct CreateInfo;

	struct BindingData {
		std::string name;
		vk::DescriptorSetLayoutBinding layoutBinding;
		Texture::Type textureType{};
	};

	DescriptorSet(not_null<VRAM*> vram, CreateInfo const& info);
	DescriptorSet(DescriptorSet&&) = default;
	DescriptorSet& operator=(DescriptorSet&&) = default;

	vk::DescriptorSet descriptorSet() const { return m_sets.get().set; }
	void swap() { m_sets.next(); }

	void updateBuffers(u32 binding, Span<Buf const> buffers);
	void updateImages(u32 binding, Span<Img const> images);
	template <typename T>
	void writeUpdate(T const& payload, u32 binding);
	void update(u32 binding, Buffer const& buffer);
	void update(u32 binding, Texture const& texture);

	Texture::Type textureType(u32 binding) const noexcept { return this->binding(binding).textureType; }
	u32 setNumber() const noexcept { return m_setNumber; }
	BindingData binding(u32 binding) const noexcept { return binding < max_bindings_v ? m_bindingData[binding] : BindingData(); }
	bool contains(u32 binding, vk::DescriptorType const* type = {}, Texture::Type const* texType = {}) const noexcept;
	bool unassigned() const noexcept { return m_sets.empty(); }

  private:
	static constexpr vk::BufferUsageFlags usage(vk::DescriptorType type) noexcept;
	void updateBuffersImpl(u32 binding, Span<Buf const> buffers, vk::DescriptorType type);
	template <typename T>
	void update(u32 binding, vk::DescriptorType type, Span<T const> writes);

	struct Binding {
		vk::DescriptorType type{};
		Texture::Type texType{};
		u32 count = 1;
	};
	struct Set {
		Binding bindings[max_bindings_v];
		std::optional<Buffer> buffer;
		vk::DescriptorSet set;
	};

	BindingData m_bindingData[max_bindings_v]{};
	TRotator<Set> m_sets{};
	u32 m_setNumber = 0;
	not_null<VRAM*> m_vram;

	std::pair<Set&, Binding&> setBind(u32 bind, u32 count = 1, vk::DescriptorType const* type = {});
};

struct DescriptorSet::CreateInfo {
	Span<BindingData const> bindingData;
	Span<vk::DescriptorSet> descriptorSets;
	u32 setNumber = 0;
};

class DescriptorPool {
  public:
	struct CreateInfo {
		ktl::fixed_vector<DescriptorSet::BindingData, max_bindings_v> bindingData{};
		vk::DescriptorSetLayout layout{};
		Buffering buffering = Buffering::eDouble;
		u32 setNumber = 0;
		u32 setsPerPool = 32;
	};

	DescriptorPool(not_null<VRAM*> vram, CreateInfo info);

	DescriptorSet& set(std::size_t index) const;
	bool unassigned() const noexcept { return m_info.bindingData.empty(); }
	void swap();

  private:
	struct Binding {
		vk::DescriptorType type{};
		Texture::Type texType{};
		u32 count = 1;
	};

	void makeSets() const;

	Binding m_bindings[16]{};
	CreateInfo m_info;
	ktl::fixed_vector<vk::DescriptorPoolSize, max_bindings_v> m_poolSizes;
	mutable std::vector<Defer<vk::DescriptorPool>> m_pools;
	mutable std::vector<DescriptorSet> m_sets;
	u32 m_maxSets{};
	not_null<VRAM*> m_vram;
	bool m_hasActiveBinding{};
};

// impl

constexpr vk::BufferUsageFlags DescriptorSet::usage(vk::DescriptorType type) noexcept {
	if (type == vk::DescriptorType::eStorageBuffer) { return vk::BufferUsageFlagBits::eStorageBuffer; }
	return vk::BufferUsageFlagBits::eUniformBuffer;
}

template <typename T>
void DescriptorSet::writeUpdate(T const& payload, u32 binding) {
	auto [s, b] = setBind(binding);
	if (!s.buffer) {
		s.buffer.emplace(m_vram->makeBO(payload, usage(b.type)));
	} else {
		s.buffer->writeT(payload);
	}
	Buf const buf{s.buffer->buffer(), sizeof(T)};
	updateBuffersImpl(binding, buf, b.type);
}

template <typename T>
void DescriptorSet::update(u32 binding, vk::DescriptorType type, Span<T const> writes) {
	vk::WriteDescriptorSet write;
	write.dstSet = descriptorSet();
	write.dstBinding = binding;
	write.dstArrayElement = 0;
	write.descriptorType = type;
	write.descriptorCount = (u32)writes.size();
	if constexpr (std::is_same_v<T, vk::DescriptorImageInfo>) {
		write.pImageInfo = writes.data();
	} else {
		write.pBufferInfo = writes.data();
	}
	m_vram->m_device->device().updateDescriptorSets(write, {});
}

inline void DescriptorSet::update(u32 binding, Buffer const& buffer) { updateBuffers(binding, Buf{buffer.buffer(), (std::size_t)buffer.writeSize()}); }
inline void DescriptorSet::update(u32 binding, Texture const& texture) { updateImages(binding, Img{texture.image().view(), texture.sampler()}); }
} // namespace le::graphics
