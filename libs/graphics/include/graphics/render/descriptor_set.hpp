#pragma once
#include <core/not_null.hpp>
#include <core/ref.hpp>
#include <core/span.hpp>
#include <graphics/render/buffering.hpp>
#include <graphics/resources.hpp>
#include <graphics/texture.hpp>
#include <graphics/utils/deferred.hpp>
#include <graphics/utils/ring_buffer.hpp>
#include <vulkan/vulkan.hpp>
#include <unordered_map>

namespace le::graphics {
class Device;
class Image;
class ShaderBuffer;

struct BindingInfo {
	vk::DescriptorSetLayoutBinding binding;
	std::string name;
};

struct SetLayoutData {
	ktl::fixed_vector<BindingInfo, 16> bindings;
	vk::DescriptorSetLayout layout;
	std::string name;
};

struct SetPoolsData {
	std::vector<SetLayoutData> sets;
	Buffering buffering = 2_B;
};

static constexpr std::size_t max_bindings_v = 16;

class DescriptorSet {
  public:
	// Combined Image Sampler
	struct CIS {
		vk::ImageView image;
		vk::Sampler sampler;
	};
	struct BO {
		vk::Buffer buffer;
		std::size_t size = 0;
		std::size_t writes = 0;
	};
	struct Imgs {
		std::vector<CIS> images;
	};
	struct Bufs {
		std::vector<BO> buffers;
		vk::DescriptorType type = vk::DescriptorType::eUniformBuffer;
	};
	struct CreateInfo;

	DescriptorSet(not_null<Device*> device, CreateInfo const& info);
	DescriptorSet(DescriptorSet&&) = default;
	DescriptorSet& operator=(DescriptorSet&&) = default;

	void index(std::size_t index);
	void swap();
	vk::DescriptorSet get() const;

	void updateBufs(u32 binding, Bufs bufs);
	bool updateImgs(u32 binding, Imgs imgs);
	template <typename C>
	void update(u32 binding, C const& buffers, vk::DescriptorType type);
	template <typename C>
	bool update(u32 binding, C const& textures);
	void update(u32 binding, Buffer const& buffer, vk::DescriptorType type);
	bool update(u32 binding, Texture const& texture);

	u32 setNumber() const noexcept;
	BindingInfo const* binding(u32 bind) const noexcept;
	bool contains(u32 bind) const noexcept;
	bool unassigned() const noexcept;

	not_null<Device*> m_device;

  private:
	template <typename T>
	void update(u32 binding, vk::DescriptorType type, Span<T const> writes);
	void update(vk::WriteDescriptorSet set);

	struct Binding {
		std::string name;
		vk::DescriptorType type;
		Bufs buffers;
		Imgs images;
		u32 count = 1;
	};
	struct Set {
		vk::DescriptorSet set;
		Deferred<vk::DescriptorPool> pool;
		Binding bindings[max_bindings_v];
	};
	struct Storage {
		vk::DescriptorSetLayout layout;
		RingBuffer<Set> setBuffer;
		BindingInfo bindingInfos[max_bindings_v];
		Buffering buffering = 1_B;
		u32 setNumber = 0;
	} m_storage;

	std::pair<Set&, Binding&> setBind(u32 bind, vk::DescriptorType type, u32 count);
};

struct DescriptorSet::CreateInfo {
	std::string_view name;
	vk::DescriptorSetLayout layout;
	Span<BindingInfo const> bindingInfos;
	Buffering buffering = 2_B;
	u32 setNumber = 0;
};

// Manages N DescriptorSet instances (drawables)
class DescriptorPool {
  public:
	DescriptorPool(not_null<Device*> device, DescriptorSet::CreateInfo const& info);

	DescriptorSet& front();
	DescriptorSet& index(std::size_t idx);
	DescriptorSet const& front() const;
	DescriptorSet const& index(std::size_t idx) const;
	Span<DescriptorSet> populate(std::size_t count);
	void swap();

	bool contains(u32 bind) const noexcept;
	bool unassigned() const noexcept;
	void clear() noexcept;

  private:
	struct Storage {
		std::string_view name;
		vk::DescriptorSetLayout layout;
		ktl::fixed_vector<BindingInfo, max_bindings_v> bindInfos;
		std::vector<DescriptorSet> descriptorSets;
		Buffering buffering;
		u32 setNumber = 0;
	} m_storage;
	not_null<Device*> m_device;
};

// Manages multiple inputs for a shader via set numbers
class ShaderInput {
  public:
	ShaderInput() = default;
	ShaderInput(not_null<VRAM*> vram, SetPoolsData data);

	DescriptorPool& pool(u32 set);
	DescriptorPool const& pool(u32 set) const;
	void swap();
	bool empty() const noexcept;
	bool contains(u32 set) const noexcept;
	bool contains(u32 set, u32 bind) const noexcept;

	bool update(Span<Texture const> textures, u32 set, u32 bind, std::size_t idx = 0);
	bool update(Span<Buffer const> buffers, u32 set, u32 bind, std::size_t idx = 0, vk::DescriptorType type = vk::DescriptorType::eUniformBuffer);
	bool update(ShaderBuffer const& buffer, u32 set, u32 bind, std::size_t idx = 0);

	DescriptorPool& operator[](u32 set);
	DescriptorPool const& operator[](u32 set) const;

	void clearSets() noexcept;

	VRAM* m_vram{};

  private:
	std::unordered_map<u32, DescriptorPool> m_setPools;
};

// impl

inline u32 DescriptorSet::setNumber() const noexcept { return m_storage.setNumber; }

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
	} else if constexpr (std::is_same_v<T, vk::DescriptorBufferInfo>) {
		write.pBufferInfo = writes.data();
	} else {
		static_assert(false_v<T>, "Invalid type");
	}
	update(write);
}
template <typename C>
void DescriptorSet::update(u32 binding, C const& buffers, vk::DescriptorType type) {
	Bufs bufs;
	for (Buffer const& buf : buffers) { bufs.buffers.push_back({buf.buffer(), (std::size_t)buf.writeSize(), buf.writeCount()}); }
	bufs.type = type;
	updateBufs(binding, std::move(bufs));
}
template <typename C>
bool DescriptorSet::update(u32 binding, C const& textures) {
	Imgs imgs;
	imgs.images.reserve(textures.size());
	for (Texture const& tex : textures) { imgs.images.push_back({tex.data().imageView, tex.data().sampler}); }
	return updateImgs(binding, std::move(imgs));
}
inline void DescriptorSet::update(u32 binding, Buffer const& buffer, vk::DescriptorType type) { update(binding, Span<Ref<Buffer const> const>(buffer), type); }
inline bool DescriptorSet::update(u32 binding, Texture const& texture) { return update(binding, Span<Ref<Texture const> const>(texture)); }
} // namespace le::graphics
