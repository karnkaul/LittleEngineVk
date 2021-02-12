#include <graphics/common.hpp>
#include <graphics/context/device.hpp>
#include <graphics/descriptor_set.hpp>
#include <graphics/pipeline.hpp>
#include <graphics/resources.hpp>
#include <graphics/texture.hpp>

namespace le::graphics {
namespace {
template <typename T, typename U, typename F>
bool stale(T&& lhs, U&& rhs, F same) {
	if (lhs.size() != rhs.size()) {
		return true;
	}
	for (std::size_t idx = 0; idx < lhs.size(); ++idx) {
		auto const& l = lhs[idx];
		auto const& r = rhs[idx];
		if (!same(l, r)) {
			return true;
		}
	}
	return false;
}
} // namespace

DescriptorSet::DescriptorSet(Device& device, CreateInfo const& info) : m_device(device) {
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
			g_log.log(lvl::info, 1, "[{}] DescriptorSet [{}] binding [{}] [{}] constructed", g_name, info.setNumber, b, bindingInfo.name);
		}
		set.pool = m_device.get().createDescriptorPool(poolSizes, 1);
		set.set = m_device.get().allocateDescriptorSets(set.pool, m_storage.layout, 1).front();
		m_storage.setBuffer.push(std::move(set));
	}
}

DescriptorSet::DescriptorSet(DescriptorSet&& rhs) noexcept : m_device(rhs.m_device), m_storage(std::exchange(rhs.m_storage, Storage())) {
}

DescriptorSet& DescriptorSet::operator=(DescriptorSet&& rhs) noexcept {
	if (&rhs != this) {
		destroy();
		m_storage = std::exchange(rhs.m_storage, Storage());
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

void DescriptorSet::updateBuffers(u32 binding, View<Ref<Buffer const>> buffers, std::size_t size, vk::DescriptorType type) {
	auto [set, bind] = setBind(binding, type, (u32)buffers.size());
	if (stale(buffers, bind.buffers, [](Buffer const& lhs, Buffer const& rhs) { return lhs.buffer() == rhs.buffer(); })) {
		bind.buffers = {buffers.begin(), buffers.end()};
		std::vector<vk::DescriptorBufferInfo> bufferInfos;
		for (auto const& buf : buffers) {
			vk::DescriptorBufferInfo bufferInfo;
			bufferInfo.buffer = buf.get().buffer();
			bufferInfo.offset = 0;
			bufferInfo.range = size;
			bufferInfos.push_back(bufferInfo);
		}
		update<vk::DescriptorBufferInfo>(binding, bind.type, bufferInfos);
	}
}

bool DescriptorSet::updateCIS(u32 binding, std::vector<CIS> cis) {
	auto [set, bind] = setBind(binding, vk::DescriptorType::eCombinedImageSampler, (u32)cis.size());
	if (stale(cis, bind.cis, [](CIS const& lhs, CIS const& rhs) { return lhs.image == rhs.image && lhs.sampler == rhs.sampler; })) {
		std::vector<vk::DescriptorImageInfo> imageInfos;
		imageInfos.reserve(cis.size());
		for (auto const& tex : cis) {
			vk::DescriptorImageInfo imageInfo;
			imageInfo.imageView = tex.image;
			imageInfo.imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
			imageInfo.sampler = tex.sampler;
			imageInfos.push_back(imageInfo);
		}
		update<vk::DescriptorImageInfo>(binding, bind.type, imageInfos);
		bind.cis = std::move(cis);
	}
	return true;
}

bool DescriptorSet::updateTextures(u32 binding, View<Texture> textures) {
	std::vector<CIS> cis;
	cis.reserve(textures.size());
	for (auto const& texture : textures) {
		cis.push_back({texture.data().imageView, texture.data().sampler});
	}
	return updateCIS(binding, std::move(cis));
}

void DescriptorSet::update(vk::WriteDescriptorSet write) {
	m_device.get().device().updateDescriptorSets(write, {});
}

void DescriptorSet::destroy() {
	Device& d = m_device;
	d.defer([b = m_storage.setBuffer, sn = m_storage.setNumber, &d]() mutable {
		for (Set& set : b.ts) {
			d.destroy(set.pool);
			if (!b.ts.empty()) {
				g_log.log(lvl::info, 1, "[{}] DescriptorSet [{}] destroyed", g_name, sn);
			}
		}
	});
	m_storage = {};
}

std::pair<DescriptorSet::Set&, DescriptorSet::Binding&> DescriptorSet::setBind(u32 bind, vk::DescriptorType type, u32 count) {
	auto& set = m_storage.setBuffer.get();
	ENSURE(set.bindings.find(bind) != set.bindings.end(), "Nonexistent binding");
	auto& binding = set.bindings[bind];
	ENSURE(binding.type == type, "Mismatched descriptor type");
	ENSURE(binding.count == count, "Mismatched descriptor size");
	return {set, binding};
}

SetPool::SetPool(Device& device, DescriptorSet::CreateInfo const& info) : m_device(device) {
	m_storage.layout = info.layout;
	m_storage.rotateCount = info.rotateCount;
	m_storage.setNumber = info.setNumber;
	for (auto const& bindInfo : info.bindingInfos) {
		m_storage.bindInfos.push_back(bindInfo);
	}
	populate(1);
}

DescriptorSet& SetPool::front() {
	populate(1);
	return m_storage.descriptorSets.front();
}

DescriptorSet& SetPool::index(std::size_t idx) {
	populate(idx + 1);
	return m_storage.descriptorSets[idx];
}

Span<DescriptorSet> SetPool::populate(std::size_t count) {
	m_storage.descriptorSets.reserve(count);
	while (m_storage.descriptorSets.size() < count) {
		DescriptorSet::CreateInfo info{m_storage.layout, m_storage.bindInfos, m_storage.rotateCount, m_storage.setNumber};
		m_storage.descriptorSets.emplace_back(m_device, info);
	}
	return Span(m_storage.descriptorSets.data(), count);
}

void SetPool::swap() {
	for (auto& descriptorSet : m_storage.descriptorSets) {
		descriptorSet.next();
	}
}

ShaderInput::ShaderInput(Pipeline const& pipe, std::size_t rotateCount) {
	m_setPools = pipe.makeSetPools(rotateCount);
}

SetPool& ShaderInput::set(u32 set) {
	if (auto it = m_setPools.find(set); it != m_setPools.end()) {
		return it->second;
	}
	ENSURE(false, "Nonexistent set");
	throw std::runtime_error("Nonexistent set");
}

SetPool const& ShaderInput::set(u32 set) const {
	if (auto it = m_setPools.find(set); it != m_setPools.end()) {
		return it->second;
	}
	ENSURE(false, "Nonexistent set");
	throw std::runtime_error("Nonexistent set");
}

void ShaderInput::swap() {
	for (auto& [_, set] : m_setPools) {
		set.swap();
	}
}

bool ShaderInput::empty() const noexcept {
	return m_setPools.empty();
}

SetPool& ShaderInput::operator[](u32 set) {
	return this->set(set);
}

SetPool const& ShaderInput::operator[](u32 set) const {
	return this->set(set);
}
} // namespace le::graphics
