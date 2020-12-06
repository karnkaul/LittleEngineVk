#include <graphics/context/device.hpp>
#include <graphics/context/vram.hpp>
#include <graphics/descriptor_set.hpp>
#include <graphics/texture.hpp>

namespace le::graphics {
namespace {
vk::BufferUsageFlags toFlags(vk::DescriptorType type) noexcept {
	switch (type) {
	case vk::DescriptorType::eStorageBuffer:
		return vk::BufferUsageFlagBits::eStorageBuffer;
	default:
		return vk::BufferUsageFlagBits::eUniformBuffer;
	}
}
} // namespace

DescriptorSet::DescriptorSet(VRAM& vram, CreateInfo const& info) : m_vram(vram), m_device(vram.m_device) {
	Device& device = m_device;
	m_storage.rotateCount = (u32)info.rotateCount;
	m_storage.layout = info.layout;
	m_storage.setNumber = info.setNumber;
	for (auto const& bindingInfo : info.bindingInfos) {
		m_storage.bindingInfos[bindingInfo.binding.binding] = bindingInfo;
	}
	std::vector<vk::DescriptorPoolSize> poolSizes;
	poolSizes.reserve(m_storage.bindingInfos.size());
	for (u32 buf = 0; buf < m_storage.rotateCount; ++buf) {
		Set set;
		for (auto const& [b, bindingInfo] : m_storage.bindingInfos) {
			u32 const totalSize = bindingInfo.binding.descriptorCount * m_storage.rotateCount;
			poolSizes.push_back({bindingInfo.binding.descriptorType, totalSize});
			set.bindings[b].type = bindingInfo.binding.descriptorType;
			set.bindings[b].count = bindingInfo.binding.descriptorCount;
			set.bindings[b].name = bindingInfo.name;
			logD("[{}] DescriptorSet [{}] binding [{}] [{}] constructed", g_name, info.setNumber, b, bindingInfo.name);
		}
		set.pool = device.createDescriptorPool(poolSizes, 1);
		set.set = device.allocateDescriptorSets(set.pool, m_storage.layout, 1).front();
		m_storage.setBuffer.push(std::move(set));
	}
}

DescriptorSet::DescriptorSet(DescriptorSet&& rhs) noexcept : m_storage(std::exchange(rhs.m_storage, Storage())), m_vram(rhs.m_vram), m_device(rhs.m_device) {
}

DescriptorSet& DescriptorSet::operator=(DescriptorSet&& rhs) noexcept {
	if (&rhs != this) {
		destroy();
		m_storage = std::exchange(rhs.m_storage, Storage());
		m_vram = rhs.m_vram;
		m_device = rhs.m_device;
	}
	return *this;
}

DescriptorSet::~DescriptorSet() {
	destroy();
}

void DescriptorSet::index(std::size_t index) {
	if (index < m_storage.rotateCount) {
		m_storage.setBuffer.index = index;
	}
}

void DescriptorSet::next() {
	m_storage.setBuffer.next();
}

vk::DescriptorSet DescriptorSet::get() const {
	return m_storage.setBuffer.get().set;
}

std::vector<CView<Buffer>> DescriptorSet::buffers(u32 binding) const {
	auto& set = m_storage.setBuffer.get();
	std::vector<CView<Buffer>> ret;
	if (auto it = set.bindings.find(binding); it != set.bindings.end()) {
		for (auto const buf : it->second.buffers) {
			ret.push_back(buf);
		}
	}
	return ret;
}

bool DescriptorSet::writeBuffers(u32 binding, void* pData, std::size_t size, std::size_t count, vk::DescriptorType type) {
	auto& set = m_storage.setBuffer.get();
	auto& bind = set.bindings[binding];
	ENSURE(bind.type == type, "Mismatched descriptor type");
	ENSURE(bind.count == (u32)count, "Mismatched descriptor count");
	VRAM& vram = m_vram;
	bool bStale = bind.buffers.size() != count;
	bind.buffers.resize((std::size_t)count);
	if (!bStale) {
		for (auto b : bind.buffers) {
			if (!b || b->writeSize < size) {
				bStale = true;
				break;
			}
		}
	}
	if (bStale) {
		std::vector<vk::DescriptorBufferInfo> bufferInfos;
		for (auto& buf : bind.buffers) {
			buf = resize(buf, size, bind.type, bind.name);
			vk::DescriptorBufferInfo bufferInfo;
			bufferInfo.buffer = buf->buffer;
			bufferInfo.offset = 0;
			bufferInfo.range = size;
			bufferInfos.push_back(bufferInfo);
		}
		vk::WriteDescriptorSet descWrite;
		descWrite.dstSet = set.set;
		descWrite.dstBinding = binding;
		descWrite.dstArrayElement = 0;
		descWrite.descriptorType = type;
		descWrite.descriptorCount = (u32)bufferInfos.size();
		descWrite.pBufferInfo = bufferInfos.data();
		Device& d = m_device;
		d.m_device.updateDescriptorSets(descWrite, {});
	}
	for (std::size_t i = 0; i < count; ++i) {
		void* pStart = (void*)((u8*)pData + i);
		if (!vram.write(bind.buffers[i], pStart, {0, size})) {
			return false;
		}
	}
	return true;
}

void DescriptorSet::updateBuffers(u32 binding, Span<CView<Buffer>> buffers, std::size_t size, vk::DescriptorType type) {
	auto& set = m_storage.setBuffer.get();
	auto& bind = set.bindings[binding];
	ENSURE(bind.type == type, "Mismatched descriptor type");
	ENSURE(bind.count == (u32)buffers.size(), "Mismatched descriptor count");
	std::vector<vk::DescriptorBufferInfo> bufferInfos;
	for (auto const& buf : buffers) {
		vk::DescriptorBufferInfo bufferInfo;
		bufferInfo.buffer = buf->buffer;
		bufferInfo.offset = 0;
		bufferInfo.range = size;
		bufferInfos.push_back(bufferInfo);
	}
	vk::WriteDescriptorSet descWrite;
	descWrite.dstSet = set.set;
	descWrite.dstBinding = binding;
	descWrite.dstArrayElement = 0;
	descWrite.descriptorType = type;
	descWrite.descriptorCount = (u32)bufferInfos.size();
	descWrite.pBufferInfo = bufferInfos.data();
	Device& d = m_device;
	d.m_device.updateDescriptorSets(descWrite, {});
}

bool DescriptorSet::updateCIS(u32 binding, std::vector<CIS> cis) {
	auto& set = m_storage.setBuffer.get();
	ENSURE(set.bindings.find(binding) != set.bindings.end(), "Nonexistent binding");
	auto& bind = set.bindings[binding];
	ENSURE(bind.type == vk::DescriptorType::eCombinedImageSampler, "Mismatched descriptor type");
	ENSURE(bind.count == (u32)cis.size(), "Mismatched descriptor size");
	bool bStale = cis.size() != bind.cis.size();
	if (!bStale) {
		for (std::size_t idx = 0; idx < bind.cis.size(); ++idx) {
			CIS const& lhs = bind.cis[idx];
			CIS const& rhs = cis[idx];
			if (lhs.image != rhs.image || lhs.sampler != rhs.sampler) {
				bStale = true;
				break;
			}
		}
	}
	if (bStale) {
		std::vector<vk::DescriptorImageInfo> imageInfos;
		imageInfos.reserve(cis.size());
		for (auto const& tex : cis) {
			vk::DescriptorImageInfo imageInfo;
			imageInfo.imageView = tex.image;
			imageInfo.imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
			imageInfo.sampler = tex.sampler;
			imageInfos.push_back(imageInfo);
		}
		vk::WriteDescriptorSet descWrite;
		descWrite.dstSet = get();
		descWrite.dstBinding = binding;
		descWrite.dstArrayElement = 0;
		descWrite.descriptorType = vk::DescriptorType::eCombinedImageSampler;
		descWrite.descriptorCount = (u32)imageInfos.size();
		descWrite.pImageInfo = imageInfos.data();
		Device& d = m_device;
		d.m_device.updateDescriptorSets(descWrite, {});
		bind.cis = std::move(cis);
	}
	return true;
}

bool DescriptorSet::updateTextures(u32 binding, Span<Texture> textures) {
	std::vector<CIS> cis;
	cis.reserve(textures.size());
	for (auto const& texture : textures) {
		cis.push_back({texture.data().imageView, texture.data().sampler});
	}
	return updateCIS(binding, std::move(cis));
}

void DescriptorSet::destroy() {
	VRAM& v = m_vram;
	Device& d = m_device;
	d.defer([b = m_storage.setBuffer, sn = m_storage.setNumber, &d, &v]() mutable {
		for (Set& set : b.ts) {
			d.destroy(set.pool);
			for (auto [_, bind] : set.bindings) {
				for (auto b : bind.buffers) {
					v.destroy(b);
				}
			}
			logD_if(!b.ts.empty(), "[{}] DescriptorSet [{}] destroyed", g_name, sn);
		}
	});
}

View<Buffer> DescriptorSet::resize(View<Buffer> old, std::size_t size, vk::DescriptorType type, std::string_view name) const {
	VRAM& vram = m_vram;
	if (old) {
		Device& device = m_device;
		device.defer([b = old, &vram]() mutable { vram.destroy(b); });
	}
	return vram.createBO(name, size, toFlags(type), true);
}

SetFactory::SetFactory(VRAM& vram, CreateInfo const& info) : m_vram(vram), m_device(vram.m_device) {
	m_storage.layout = info.layout;
	m_storage.rotateCount = info.rotateCount;
	m_storage.setNumber = info.setNumber;
	for (auto const& bindInfo : info.bindInfos) {
		m_storage.bindInfos.push_back(bindInfo);
	}
	populate(1);
}

DescriptorSet& SetFactory::front() {
	ENSURE(!m_storage.descriptorSets.empty(), "Invariant violated");
	return m_storage.descriptorSets.front();
}

DescriptorSet& SetFactory::at(std::size_t idx) {
	populate(idx + 1);
	return m_storage.descriptorSets[idx];
}

RWSpan<DescriptorSet> SetFactory::populate(std::size_t count) {
	m_storage.descriptorSets.reserve(count);
	while (m_storage.descriptorSets.size() < count) {
		DescriptorSet::CreateInfo info{m_storage.layout, m_storage.bindInfos, m_storage.rotateCount, m_storage.setNumber};
		m_storage.descriptorSets.emplace_back(m_vram, info);
	}
	return RWSpan(m_storage.descriptorSets.data(), count);
}

void SetFactory::swap() {
	for (auto& descriptorSet : m_storage.descriptorSets) {
		descriptorSet.next();
	}
}
} // namespace le::graphics
