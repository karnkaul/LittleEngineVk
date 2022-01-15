#pragma once
#include <levk/graphics/memory.hpp>
#include <levk/graphics/texture.hpp>
#include <levk/graphics/utils/trotator.hpp>

namespace le::graphics {
class ShaderBuffer;

struct SetBindingData {
	std::string name;
	vk::DescriptorSetLayoutBinding binding;
	Texture::Type textureType{};
};

struct SetLayoutData {
	ktl::fixed_vector<SetBindingData, 16> bindingData;
	vk::DescriptorSetLayout layout;
};

struct SetPoolsData {
	std::vector<SetLayoutData> sets;
	Buffering buffering = Buffering::eDouble;
};

static constexpr std::size_t max_bindings_v = 16;

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

	DescriptorSet(not_null<VRAM*> vram, CreateInfo const& info);
	DescriptorSet(DescriptorSet&&) = default;
	DescriptorSet& operator=(DescriptorSet&&) = default;

	void swap();
	vk::DescriptorSet get() const;

	void updateBuffers(u32 binding, Span<Buf const> buffers);
	void updateImages(u32 binding, Span<Img const> images);
	template <typename T>
	void writeUpdate(T const& payload, u32 binding);
	void update(u32 binding, Buffer const& buffer);
	void update(u32 binding, Texture const& texture);

	u32 setNumber() const noexcept { return m_storage.setNumber; }
	SetBindingData binding(u32 bind) const noexcept;
	bool contains(u32 bind, vk::DescriptorType const* type = {}, Texture::Type const* texType = {}) const noexcept;
	bool unassigned() const noexcept;

  private:
	static constexpr vk::BufferUsageFlags usage(vk::DescriptorType type) noexcept;
	void updateBuffersImpl(u32 binding, Span<Buf const> buffers, vk::DescriptorType type);
	template <typename T>
	void update(u32 binding, vk::DescriptorType type, Span<T const> writes);
	void update(vk::WriteDescriptorSet write) { m_vram->m_device->device().updateDescriptorSets(write, {}); }

	struct Binding {
		vk::DescriptorType type{};
		Texture::Type texType{};
		u32 count = 1;
	};
	struct Set {
		Binding bindings[max_bindings_v];
		std::optional<Buffer> buffer;
		Defer<vk::DescriptorPool> pool;
		vk::DescriptorSet set;
	};
	struct Storage {
		SetBindingData bindingData[max_bindings_v];
		TRotator<Set> sets;
		vk::DescriptorSetLayout layout;
		u32 setNumber = 0;
		Buffering buffering{};
	} m_storage;
	not_null<VRAM*> m_vram;

	std::pair<Set&, Binding&> setBind(u32 bind, u32 count = 1, vk::DescriptorType const* type = {});
};

struct DescriptorSet::CreateInfo {
	vk::DescriptorSetLayout layout;
	Span<SetBindingData const> bindingData;
	Buffering buffering = Buffering::eDouble;
	u32 setNumber = 0;
};

// Manages multiple inputs for a shader via set numbers
class ShaderInput {
  public:
	ShaderInput() = default;
	ShaderInput(not_null<VRAM*> vram, SetPoolsData data);

	DescriptorSet& set(u32 set, std::size_t index);
	void swap();
	bool empty() const noexcept;
	bool contains(u32 set) const noexcept;
	bool contains(u32 set, u32 bind, vk::DescriptorType const* type = {}, Texture::Type const* texType = {}) const noexcept;
	Texture::Type textureType(u32 set, u32 bind) const noexcept;

	void update(ShaderBuffer const& buffer, u32 set, u32 bind, std::size_t idx);
	void clear() noexcept;

	VRAM* m_vram{};

  private:
	struct Pool {
		vk::DescriptorSetLayout layout;
		ktl::fixed_vector<SetBindingData, max_bindings_v> bindingData;
		mutable std::vector<DescriptorSet> descriptorSets;
		u32 setNumber = 0;
		Buffering buffering;
		VRAM* vram{};

		void push() const;
		DescriptorSet& index(std::size_t i) const;
	};
	std::unordered_map<u32, Pool> m_setPools;
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
	write.dstSet = get();
	write.dstBinding = binding;
	write.dstArrayElement = 0;
	write.descriptorType = type;
	write.descriptorCount = (u32)writes.size();
	if constexpr (std::is_same_v<T, vk::DescriptorImageInfo>) {
		write.pImageInfo = writes.data();
	} else {
		write.pBufferInfo = writes.data();
	}
	update(write);
}

inline void DescriptorSet::update(u32 binding, Buffer const& buffer) { updateBuffers(binding, Buf{buffer.buffer(), (std::size_t)buffer.writeSize()}); }
inline void DescriptorSet::update(u32 binding, Texture const& texture) { updateImages(binding, Img{texture.image().view(), texture.sampler()}); }
} // namespace le::graphics
