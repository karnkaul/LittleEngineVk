#include <levk/graphics/render/descriptor_set.hpp>
#include <levk/graphics/render/shader_buffer.hpp>
#include <algorithm>

namespace le::graphics {
DescriptorSet::DescriptorSet(not_null<VRAM*> vram, CreateInfo const& info) : m_vram(vram) {
	m_storage.buffering = info.buffering;
	m_storage.layout = info.layout;
	m_storage.setNumber = info.setNumber;
	bool active = false;
	for (auto const& binding : info.bindingData) {
		m_storage.bindingData[binding.binding.binding] = binding;
		active |= binding.binding.descriptorType != vk::DescriptorType();
	}
	if (active) {
		std::vector<vk::DescriptorPoolSize> poolSizes;
		poolSizes.reserve(max_bindings_v);
		for (Buffering buf{}; buf < m_storage.buffering; ++buf) {
			Set set;
			std::size_t idx{};
			for (auto const& binding : m_storage.bindingData) {
				if (binding.binding.descriptorType != vk::DescriptorType()) {
					u32 const totalSize = binding.binding.descriptorCount * u32(m_storage.buffering);
					poolSizes.push_back({binding.binding.descriptorType, totalSize});
					set.bindings[idx].type = binding.binding.descriptorType;
					set.bindings[idx].count = binding.binding.descriptorCount;
					set.bindings[idx].texType = binding.textureType;
				}
				++idx;
			}
			set.pool = set.pool.make(m_vram->m_device->makeDescriptorPool(poolSizes, 1U), m_vram->m_device);
			set.set = m_vram->m_device->allocateDescriptorSets(set.pool, m_storage.layout, 1).front();
			m_storage.sets.emplace(std::move(set));
		}
	}
}

void DescriptorSet::swap() { m_storage.sets.next(); }

vk::DescriptorSet DescriptorSet::get() const { return m_storage.sets.get().set; }

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

SetBindingData DescriptorSet::binding(u32 bind) const noexcept { return bind < max_bindings_v ? m_storage.bindingData[bind] : SetBindingData(); }

bool DescriptorSet::contains(u32 bind, vk::DescriptorType const* type, Texture::Type const* texType) const noexcept {
	auto const ret = binding(bind);
	if (ret.binding == vk::DescriptorSetLayoutBinding()) { return false; }
	if (type && ret.binding.descriptorType != *type) { return false; }
	if (texType && ret.textureType != *texType) { return false; }
	return true;
}

bool DescriptorSet::unassigned() const noexcept {
	auto const& bi = m_storage.bindingData;
	return std::all_of(std::begin(bi), std::end(bi), [](SetBindingData const& bi) { return bi.binding == vk::DescriptorSetLayoutBinding(); });
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
	auto& set = m_storage.sets.get();
	ENSURE(contains(bind), "Nonexistent binding");
	auto& binding = set.bindings[bind];
	if (type) { ENSURE(binding.type == *type, "Mismatched descriptor type"); }
	ENSURE(binding.count == count, "Mismatched descriptor size");
	return {set, binding};
}

void ShaderInput::Pool::push() const { descriptorSets.emplace_back(vram, DescriptorSet::CreateInfo{layout, bindingData, buffering, setNumber}); }

DescriptorSet& ShaderInput::Pool::index(std::size_t idx) const {
	descriptorSets.reserve(idx + 1);
	while (descriptorSets.size() < idx + 1) { push(); }
	return descriptorSets[idx];
}

DescriptorSet& ShaderInput::set(u32 set, std::size_t index) {
	if (auto it = m_setPools.find(set); it != m_setPools.end()) { return it->second.index(index); }
	ENSURE(false, "Nonexistent set");
}

void ShaderInput::swap() {
	for (auto& [_, pool] : m_setPools) {
		for (auto& set : pool.descriptorSets) { set.swap(); }
	}
}

bool ShaderInput::empty() const noexcept { return m_setPools.empty(); }

bool ShaderInput::contains(u32 set) const noexcept {
	if (auto it = m_setPools.find(set); it != m_setPools.end()) { return !it->second.index(0).unassigned(); }
	return false;
}

bool ShaderInput::contains(u32 set, u32 bind, vk::DescriptorType const* type, Texture::Type const* texType) const noexcept {
	if (auto it = m_setPools.find(set); it != m_setPools.end()) { return it->second.index(0).contains(bind, type, texType); }
	return false;
}

Texture::Type ShaderInput::textureType(u32 set, u32 bind) const noexcept {
	if (auto it = m_setPools.find(set); it != m_setPools.end()) { return it->second.index(0).binding(bind).textureType; }
	return {};
}

ShaderInput::ShaderInput(not_null<VRAM*> vram, SetPoolsData data) : m_vram(vram) {
	Buffering const buffering = data.buffering == Buffering::eNone ? Buffering::eDouble : data.buffering;
	for (std::size_t set = 0; set < data.sets.size(); ++set) {
		Pool pool;
		pool.vram = m_vram;
		auto const& sld = data.sets[set];
		pool.layout = sld.layout;
		pool.buffering = buffering;
		pool.setNumber = (u32)set;
		pool.bindingData = std::move(sld.bindingData);
		m_setPools.emplace((u32)set, std::move(pool));
	}
}

void ShaderInput::update(ShaderBuffer const& buffer, u32 set, u32 bind, std::size_t idx) { buffer.update(this->set(set, idx), bind); }

void ShaderInput::clear() noexcept {
	for (auto& [_, pool] : m_setPools) { pool.descriptorSets.clear(); }
}
} // namespace le::graphics
