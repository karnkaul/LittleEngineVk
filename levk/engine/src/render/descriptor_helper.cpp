#include <levk/core/utils/enumerate.hpp>
#include <levk/core/utils/expect.hpp>
#include <levk/engine/assets/asset_store.hpp>
#include <levk/engine/render/descriptor_helper.hpp>
#include <levk/graphics/render/pipeline_factory.hpp>
#include <levk/graphics/render/shader_buffer.hpp>

namespace le {
DescriptorUpdater::DescriptorUpdater(not_null<Cache const*> cache, not_null<DescriptorSet*> descriptorSet) : m_cache(cache), m_descriptorSet(descriptorSet) {}

bool DescriptorUpdater::update(u32 binding, ShaderBuffer const& buffer) const {
	if (check(binding)) {
		buffer.update(*m_descriptorSet, binding);
		return true;
	}
	return false;
}

bool DescriptorUpdater::update(u32 binding, Opt<Texture const> tex, TextureFallback fb) const {
	if (check(binding)) {
		m_descriptorSet->update(binding, safeTex(tex, binding, fb));
		return true;
	}
	return false;
}

bool DescriptorUpdater::check(u32 bind, vk::DescriptorType const* type, Texture::Type const* texType) const noexcept {
	if (!valid()) { return false; }
	for (u32 const b : m_binds) {
		if (bind == b) { return true; }
	}
	if (m_descriptorSet->contains(bind, type, texType)) {
		m_binds.push_back(bind);
		return true;
	}
	return false;
}

graphics::Texture const& DescriptorUpdater::safeTex(Texture const* tex, u32 bind, TextureFallback fb) const {
	auto const texType = m_descriptorSet->textureType(bind);
	if (tex && tex->ready() && tex->type() == texType) { return *tex; }
	if (texType == Texture::Type::eCube) { return *m_cache->cube; }
	return *m_cache->defaults[fb];
}

bool DescriptorMap::contains(u32 setNumber) { return m_input->contains(setNumber); }

DescriptorUpdater DescriptorMap::nextSet(DrawBindings const& bindings, u32 setNumber) {
	if (contains(setNumber)) {
		auto const index = m_nextIndex[setNumber]++;
		bindings.bind(setNumber, index);
		return DescriptorUpdater(m_cache, &m_input->set(setNumber, index));
	}
	return {};
}

void DescriptorBinder::bind(DrawBindings const& indices) const {
	for (auto const [di, set] : utils::enumerate(indices.indices)) {
		if (di && m_input->contains((u32)set)) { m_cb.bindSet(m_layout, m_input->set((u32)set, *di)); }
	}
}

DescriptorHelper::Cache DescriptorHelper::Cache::make(not_null<const AssetStore*> store) {
	decltype(Cache::defaults) defaults = {
		store->find<Texture>("textures/white"),
		store->find<Texture>("textures/black"),
		store->find<Texture>("textures/magenta"),
	};
	return Cache{defaults, store->find<Texture>("cubemaps/blank")};
}
} // namespace le
