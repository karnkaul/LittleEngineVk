#pragma once
#include <unordered_map>
#include <core/not_null.hpp>
#include <core/ref.hpp>
#include <core/span.hpp>
#include <graphics/resources.hpp>
#include <graphics/texture.hpp>
#include <graphics/utils/ring_buffer.hpp>
#include <vulkan/vulkan.hpp>

namespace le::graphics {
class Device;
class Image;
class Pipeline;
class ShaderBuffer;

struct BindingInfo {
	vk::DescriptorSetLayoutBinding binding;
	std::string name;
	bool bUnassigned = false;
};

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
	DescriptorSet(DescriptorSet&&) noexcept;
	DescriptorSet& operator=(DescriptorSet&&) noexcept;
	~DescriptorSet();

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
	void update(u32 binding, vk::DescriptorType type, View<T> writes);
	void update(vk::WriteDescriptorSet set);
	void destroy();

	struct Binding {
		std::string name;
		vk::DescriptorType type;
		Bufs buffers;
		Imgs images;
		u32 count = 1;
	};
	struct Set {
		vk::DescriptorSet set;
		vk::DescriptorPool pool;
		std::unordered_map<u32, Binding> bindings;
	};
	struct Storage {
		vk::DescriptorSetLayout layout;
		RingBuffer<Set> setBuffer;
		std::unordered_map<u32, BindingInfo> bindingInfos;
		u32 rotateCount = 1;
		u32 setNumber = 0;
	} m_storage;

	std::pair<Set&, Binding&> setBind(u32 bind, vk::DescriptorType type, u32 count);
};

struct DescriptorSet::CreateInfo {
	std::string_view name;
	vk::DescriptorSetLayout layout;
	View<BindingInfo> bindingInfos;
	std::size_t rotateCount = 2;
	u32 setNumber = 0;
};

// Manages N DescriptorSet instances (drawables)
class SetPool {
  public:
	SetPool(not_null<Device*> device, DescriptorSet::CreateInfo const& info);

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
		std::vector<BindingInfo> bindInfos;
		std::vector<DescriptorSet> descriptorSets;
		std::size_t rotateCount = 0;
		u32 setNumber = 0;
	} m_storage;
	not_null<Device*> m_device;
};

// Manages multiple inputs for a shader via set numbers
class ShaderInput {
  public:
	ShaderInput() = default;
	ShaderInput(Pipeline const& pipe, std::size_t rotateCount);

	SetPool& set(u32 set);
	SetPool const& set(u32 set) const;
	void swap();
	bool empty() const noexcept;
	bool contains(u32 set) const noexcept;
	bool contains(u32 set, u32 bind) const noexcept;

	bool update(View<Texture> textures, u32 set, u32 bind, std::size_t idx = 0);
	bool update(View<Buffer> buffers, u32 set, u32 bind, std::size_t idx = 0, vk::DescriptorType type = vk::DescriptorType::eUniformBuffer);
	bool update(ShaderBuffer const& buffer, u32 set, u32 bind, std::size_t idx = 0);

	SetPool& operator[](u32 set);
	SetPool const& operator[](u32 set) const;

	void clearSets() noexcept;

  private:
	std::unordered_map<u32, SetPool> m_setPools;
};

// impl

inline u32 DescriptorSet::setNumber() const noexcept {
	return m_storage.setNumber;
}

template <typename T>
void DescriptorSet::update(u32 binding, vk::DescriptorType type, View<T> writes) {
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
	for (Buffer const& buf : buffers) {
		bufs.buffers.push_back({buf.buffer(), (std::size_t)buf.writeSize(), buf.writeCount()});
	}
	bufs.type = type;
	updateBufs(binding, std::move(bufs));
}
template <typename C>
bool DescriptorSet::update(u32 binding, C const& textures) {
	Imgs imgs;
	imgs.images.reserve(textures.size());
	for (Texture const& tex : textures) {
		imgs.images.push_back({tex.data().imageView, tex.data().sampler});
	}
	return updateImgs(binding, std::move(imgs));
}
inline void DescriptorSet::update(u32 binding, Buffer const& buffer, vk::DescriptorType type) {
	update(binding, View<Ref<Buffer const>>(buffer), type);
}
inline bool DescriptorSet::update(u32 binding, Texture const& texture) {
	return update(binding, View<Ref<Texture const>>(texture));
}
} // namespace le::graphics
