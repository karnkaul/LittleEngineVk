#include <core/utils/algo.hpp>
#include <graphics/common.hpp>
#include <graphics/context/device.hpp>
#include <graphics/render/descriptor_set.hpp>
#include <graphics/render/pipeline.hpp>
#include <graphics/render/shader_buffer.hpp>
#include <graphics/resources.hpp>
#include <graphics/texture.hpp>

namespace le::graphics {
namespace {
bool stale(DescriptorSet::Imgs const& lhs, DescriptorSet::Imgs const& rhs) noexcept {
	if (lhs.images.size() != rhs.images.size()) { return true; }
	for (std::size_t idx = 0; idx < lhs.images.size(); ++idx) {
		auto const& l = lhs.images[idx];
		auto const& r = rhs.images[idx];
		if (l.image != r.image || l.sampler != r.sampler) { return true; }
	}
	return false;
}

bool stale(DescriptorSet::Bufs const& lhs, DescriptorSet::Bufs const& rhs) noexcept {
	if (lhs.buffers.size() != rhs.buffers.size() || lhs.type != rhs.type) { return true; }
	for (std::size_t idx = 0; idx < lhs.buffers.size(); ++idx) {
		auto const& l = lhs.buffers[idx];
		auto const& r = rhs.buffers[idx];
		if (l.size != r.size || l.writes != r.writes || l.buffer != r.buffer) { return true; }
	}
	return false;
}
} // namespace

DescriptorSet::DescriptorSet(not_null<Device*> device, CreateInfo const& info) : m_device(device) {
	m_storage.buffering = info.buffering;
	m_storage.layout = info.layout;
	m_storage.setNumber = info.setNumber;
	bool bActive = false;
	for (auto const& bindingInfo : info.bindingInfos) {
		m_storage.bindingInfos[bindingInfo.binding.binding] = bindingInfo;
		bActive |= !bindingInfo.bUnassigned;
	}
	if (bActive) {
		std::vector<vk::DescriptorPoolSize> poolSizes;
		poolSizes.reserve(m_storage.bindingInfos.size());
		for (Buffering buf{}; buf < m_storage.buffering; ++buf.value) {
			Set set;
			for (auto const& [b, bindingInfo] : m_storage.bindingInfos) {
				if (!bindingInfo.bUnassigned) {
					u32 const totalSize = bindingInfo.binding.descriptorCount * m_storage.buffering.value;
					poolSizes.push_back({bindingInfo.binding.descriptorType, totalSize});
					set.bindings[b].type = bindingInfo.binding.descriptorType;
					set.bindings[b].count = bindingInfo.binding.descriptorCount;
					set.bindings[b].name = bindingInfo.name;
				}
			}
			set.pool = makeDeferred<vk::DescriptorPool>(m_device, poolSizes, 1U);
			set.set = m_device->allocateDescriptorSets(*set.pool, m_storage.layout, 1).front();
			m_storage.setBuffer.emplace(std::move(set));
		}
	}
}

void DescriptorSet::index(std::size_t index) {
	if (index < m_storage.buffering.value) { m_storage.setBuffer.index = index; }
}

void DescriptorSet::swap() { m_storage.setBuffer.next(); }

vk::DescriptorSet DescriptorSet::get() const { return m_storage.setBuffer.get().set; }

void DescriptorSet::updateBufs(u32 binding, Bufs bufs) {
	auto [set, bind] = setBind(binding, bufs.type, (u32)bufs.buffers.size());
	if (stale(bufs, bind.buffers)) {
		std::vector<vk::DescriptorBufferInfo> bufferInfos;
		for (auto const& buf : bufs.buffers) {
			vk::DescriptorBufferInfo bufferInfo;
			bufferInfo.buffer = buf.buffer;
			bufferInfo.offset = 0;
			bufferInfo.range = buf.size;
			bufferInfos.push_back(bufferInfo);
		}
		bind.buffers = std::move(bufs);
		update<vk::DescriptorBufferInfo>(binding, bind.type, bufferInfos);
	}
}

bool DescriptorSet::updateImgs(u32 binding, Imgs imgs) {
	auto [set, bind] = setBind(binding, vk::DescriptorType::eCombinedImageSampler, (u32)imgs.images.size());
	if (stale(imgs, bind.images)) {
		std::vector<vk::DescriptorImageInfo> imageInfos;
		imageInfos.reserve(imgs.images.size());
		for (auto const& tex : imgs.images) {
			vk::DescriptorImageInfo imageInfo;
			imageInfo.imageView = tex.image;
			imageInfo.imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
			imageInfo.sampler = tex.sampler;
			imageInfos.push_back(imageInfo);
		}
		update<vk::DescriptorImageInfo>(binding, bind.type, imageInfos);
		bind.images = std::move(imgs);
	}
	return true;
}

BindingInfo const* DescriptorSet::binding(u32 bind) const noexcept {
	if (auto it = m_storage.bindingInfos.find(bind); it != m_storage.bindingInfos.end()) { return &it->second; }
	return nullptr;
}

bool DescriptorSet::contains(u32 bind) const noexcept {
	if (auto b = binding(bind)) { return b && !b->bUnassigned; }
	return false;
}

bool DescriptorSet::unassigned() const noexcept {
	auto const& bi = m_storage.bindingInfos;
	return bi.empty() || std::all_of(bi.begin(), bi.end(), [](auto const& kvp) { return kvp.second.bUnassigned; });
}

void DescriptorSet::update(vk::WriteDescriptorSet write) { m_device->device().updateDescriptorSets(write, {}); }

std::pair<DescriptorSet::Set&, DescriptorSet::Binding&> DescriptorSet::setBind(u32 bind, vk::DescriptorType type, u32 count) {
	auto& set = m_storage.setBuffer.get();
	ensure(utils::contains(set.bindings, bind), "Nonexistent binding");
	auto& binding = set.bindings[bind];
	ensure(binding.type == type, "Mismatched descriptor type");
	ensure(binding.count == count, "Mismatched descriptor size");
	return {set, binding};
}

DescriptorPool::DescriptorPool(not_null<Device*> device, DescriptorSet::CreateInfo const& info) : m_device(device) {
	m_storage.layout = info.layout;
	m_storage.buffering = info.buffering;
	m_storage.setNumber = info.setNumber;
	bool bActive = false;
	for (auto const& bi : info.bindingInfos) {
		m_storage.bindInfos.push_back(bi);
		bActive |= !bi.bUnassigned;
		if (!bi.bUnassigned) {
			u32 const b = bi.binding.binding;
			g_log.log(lvl::debug, 2, "[{}] Binding [{}/{}] [{}] for [{}] registered", g_name, info.setNumber, b, bi.name, info.name);
		}
	}
	populate(1);
	std::string_view const suffix = bActive ? "constructed" : "inactive";
	g_log.log(lvl::debug, 2, "[{}] SetPool [{}/{}] {}", g_name, info.name, info.setNumber, suffix);
}

DescriptorSet& DescriptorPool::front() {
	populate(1);
	return m_storage.descriptorSets.front();
}

DescriptorSet& DescriptorPool::index(std::size_t idx) {
	populate(idx + 1);
	return m_storage.descriptorSets[idx];
}

DescriptorSet const& DescriptorPool::front() const {
	ensure(!m_storage.descriptorSets.empty(), "Nonexistent set index!");
	return m_storage.descriptorSets.front();
}

DescriptorSet const& DescriptorPool::index(std::size_t idx) const {
	ensure(m_storage.descriptorSets.size() > idx, "Nonexistent set index!");
	return m_storage.descriptorSets[idx];
}

Span<DescriptorSet> DescriptorPool::populate(std::size_t count) {
	m_storage.descriptorSets.reserve(count);
	while (m_storage.descriptorSets.size() < count) {
		DescriptorSet::CreateInfo info{m_storage.name, m_storage.layout, m_storage.bindInfos, m_storage.buffering, m_storage.setNumber};
		m_storage.descriptorSets.emplace_back(m_device, info);
	}
	return Span(m_storage.descriptorSets.data(), count);
}

void DescriptorPool::swap() {
	for (auto& descriptorSet : m_storage.descriptorSets) { descriptorSet.swap(); }
}

bool DescriptorPool::contains(u32 bind) const noexcept {
	if (!m_storage.descriptorSets.empty()) { return m_storage.descriptorSets.front().contains(bind); }
	if (bind < (u32)m_storage.bindInfos.size()) {
		auto const& bi = m_storage.bindInfos[(std::size_t)bind];
		return bi.binding.binding == bind && !bi.bUnassigned;
	}
	return false;
}

bool DescriptorPool::unassigned() const noexcept {
	if (!m_storage.descriptorSets.empty()) { return m_storage.descriptorSets.front().unassigned(); }
	auto const& bi = m_storage.bindInfos;
	return bi.empty() || std::all_of(bi.begin(), bi.end(), [](BindingInfo const& b) { return b.bUnassigned; });
}

void DescriptorPool::clear() noexcept { m_storage.descriptorSets.clear(); }

ShaderInput::ShaderInput(Pipeline const& pipe, Buffering buffering) : m_vram(pipe.m_vram) { m_setPools = pipe.makeSetPools(buffering); }

DescriptorPool& ShaderInput::pool(u32 set) {
	if (auto it = m_setPools.find(set); it != m_setPools.end()) { return it->second; }
	ensure(false, "Nonexistent set");
	throw std::runtime_error("Nonexistent set");
}

DescriptorPool const& ShaderInput::pool(u32 set) const {
	if (auto it = m_setPools.find(set); it != m_setPools.end()) { return it->second; }
	ensure(false, "Nonexistent set");
	throw std::runtime_error("Nonexistent set");
}

void ShaderInput::swap() {
	for (auto& [_, set] : m_setPools) { set.swap(); }
}

bool ShaderInput::empty() const noexcept { return m_setPools.empty(); }

bool ShaderInput::contains(u32 set) const noexcept {
	if (auto it = m_setPools.find(set); it != m_setPools.end()) { return !it->second.unassigned(); }
	return false;
}

bool ShaderInput::contains(u32 set, u32 bind) const noexcept {
	if (auto it = m_setPools.find(set); it != m_setPools.end()) { return it->second.contains(bind); }
	return false;
}

bool ShaderInput::update(Span<Texture const> textures, u32 set, u32 bind, std::size_t idx) {
	if constexpr (levk_debug) {
		if (contains(set)) {
			DescriptorSet& ds = this->pool(set).index(idx);
			if (auto pInfo = ds.binding(bind); pInfo && pInfo->binding.descriptorType == vk::DescriptorType::eCombinedImageSampler) {
				ds.update(bind, textures);
				return true;
			}
		}
		ensure(false, "DescriptorSet update failure");
		return false;
	} else {
		this->pool(set).index(idx).update(bind, textures);
		return true;
	}
}

bool ShaderInput::update(Span<Buffer const> buffers, u32 set, u32 bind, std::size_t idx, vk::DescriptorType type) {
	if constexpr (levk_debug) {
		if (!contains(set)) {
			ensure(false, "DescriptorSet update failure");
			return false;
		}
	}
	if (buffers.empty()) { return false; }
	this->pool(set).index(idx).update(bind, buffers, type);
	return true;
}

bool ShaderInput::update(ShaderBuffer const& buffer, u32 set, u32 bind, std::size_t idx) {
	if constexpr (levk_debug) {
		if (contains(set)) {
			DescriptorSet& ds = this->pool(set).index(idx);
			if (auto pInfo = ds.binding(bind); pInfo && pInfo->binding.descriptorType == buffer.type()) {
				buffer.update(ds, bind);
				return true;
			}
		}
		ensure(false, "DescriptorSet update failure");
		return false;
	} else {
		buffer.update(this->pool(set).index(idx), bind);
		return true;
	}
}

DescriptorPool& ShaderInput::operator[](u32 set) { return this->pool(set); }

DescriptorPool const& ShaderInput::operator[](u32 set) const { return pool(set); }

void ShaderInput::clearSets() noexcept {
	for (auto& [_, pool] : m_setPools) { pool.clear(); }
}
} // namespace le::graphics
