#include <ktl/enumerate.hpp>
#include <levk/graphics/render/descriptor_set.hpp>
#include <algorithm>

namespace le::graphics {
DescriptorSet::DescriptorSet(not_null<VRAM*> vram, CreateInfo const& info) : m_vram(vram) {
	m_setNumber = info.setNumber;
	bool active = false;
	for (auto const& data : info.bindingData) {
		m_bindingData[data.layoutBinding.binding] = data;
		active |= data.layoutBinding.descriptorType != vk::DescriptorType();
	}
	if (active) {
		for (auto const dset : info.descriptorSets) {
			Set set;
			for (auto const& [data, idx] : ktl::enumerate(m_bindingData)) {
				if (data.layoutBinding.descriptorType != vk::DescriptorType()) {
					set.bindings[idx] = {{}, data.layoutBinding.descriptorType, data.textureType, data.layoutBinding.descriptorCount};
				}
			}
			set.set = dset;
			m_sets.emplace(std::move(set));
		}
	}
}

void DescriptorSet::updateBuffers(u32 binding, Span<Buf const> buffers) {
	auto [_, b] = setBind(binding, (u32)buffers.size());
	updateBuffersImpl(binding, buffers, b.type);
}

void DescriptorSet::updateImages(u32 binding, Span<Img const> images) {
	static auto const s_type = vk::DescriptorType::eCombinedImageSampler;
	auto [set, bind] = setBind(binding, (u32)images.size(), &s_type);
	std::vector<vk::DescriptorImageInfo> imageInfos;
	imageInfos.reserve(images.size());
	for (auto const& img : images) {
		vk::DescriptorImageInfo imageInfo;
		imageInfo.imageView = img.image;
		imageInfo.imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
		imageInfo.sampler = img.sampler;
		imageInfos.push_back(imageInfo);
	}
	update<vk::DescriptorImageInfo>(binding, bind.type, imageInfos);
}

bool DescriptorSet::contains(u32 bind, vk::DescriptorType const* type, Texture::Type const* texType) const noexcept {
	auto const ret = binding(bind);
	if (ret.layoutBinding == vk::DescriptorSetLayoutBinding()) { return false; }
	if (type && ret.layoutBinding.descriptorType != *type) { return false; }
	if (texType && ret.textureType != *texType) { return false; }
	return true;
}

void DescriptorSet::updateBuffersImpl(u32 binding, Span<Buf const> buffers, vk::DescriptorType type) {
	std::vector<vk::DescriptorBufferInfo> bufferInfos;
	for (auto const& buf : buffers) {
		vk::DescriptorBufferInfo bufferInfo;
		bufferInfo.buffer = buf.buffer;
		bufferInfo.offset = 0;
		bufferInfo.range = buf.size;
		bufferInfos.push_back(bufferInfo);
	}
	update<vk::DescriptorBufferInfo>(binding, type, bufferInfos);
}

auto DescriptorSet::setBind(u32 bind, u32 count, vk::DescriptorType const* type) -> std::pair<Set&, Binding&> {
	auto& set = m_sets.get();
	ENSURE(contains(bind), "Nonexistent binding");
	auto& binding = set.bindings[bind];
	if (type) { ENSURE(binding.type == *type, "Mismatched descriptor type"); }
	ENSURE(binding.count == count, "Mismatched descriptor size");
	return {set, binding};
}

DescriptorPool::DescriptorPool(not_null<VRAM*> vram, CreateInfo info) : m_info(std::move(info)), m_vram(vram) {
	auto const f = [](DescriptorSet::BindingData const& d) { return d.layoutBinding.descriptorType != vk::DescriptorType(); };
	m_hasActiveBinding = std::any_of(m_info.bindingData.begin(), m_info.bindingData.end(), f);
	EXPECT(m_info.bindingData.empty() || m_hasActiveBinding);
	if (m_info.buffering == Buffering::eNone) { m_info.buffering = Buffering::eDouble; }
	if (m_info.setsPerPool == 0U) { m_info.setsPerPool = 1U; }
	for (auto const& [data, idx] : ktl::enumerate(m_info.bindingData)) {
		if (data.layoutBinding.descriptorType != vk::DescriptorType()) {
			u32 const totalSize = data.layoutBinding.descriptorCount * u32(m_info.buffering) * m_info.setsPerPool;
			m_maxSets += totalSize;
			m_poolSizes.push_back({data.layoutBinding.descriptorType, totalSize});
			m_bindings[idx].type = data.layoutBinding.descriptorType;
			m_bindings[idx].count = data.layoutBinding.descriptorCount;
			m_bindings[idx].texType = data.textureType;
		}
	}
}

DescriptorSet& DescriptorPool::set(std::size_t index) const {
	EXPECT(!unassigned() && m_hasActiveBinding);
	while (index >= m_sets.size()) { makeSets(); }
	EXPECT(index < m_sets.size());
	return m_sets[index];
}

void DescriptorPool::swap() {
	for (auto& set : m_sets) { set.swap(); }
}

void DescriptorPool::makeSets() const {
	EXPECT(!unassigned() && m_hasActiveBinding);
	auto pool = m_vram->m_device->makeDescriptorPool(m_poolSizes, m_maxSets);
	m_pools.push_back(Defer<vk::DescriptorPool>(pool, m_vram->m_device));
	std::vector<vk::DescriptorSetLayout> layouts(m_info.setsPerPool * std::size_t(m_info.buffering), m_info.layout);
	auto const count = m_info.setsPerPool * u32(m_info.buffering);
	auto sets = m_vram->m_device->allocateDescriptorSets(*m_pools.back(), layouts, count);
	while (!sets.empty()) {
		EXPECT(sets.size() % count == 0U);
		auto const span = Span(sets.data() + sets.size() - count, count);
		DescriptorSet::CreateInfo dci;
		dci.descriptorSets = span;
		dci.bindingData = m_info.bindingData;
		dci.setNumber = m_info.setNumber;
		m_sets.push_back(DescriptorSet(m_vram, dci));
		for (u32 i = 0; i < count; ++i) { sets.pop_back(); }
	}
}
} // namespace le::graphics
