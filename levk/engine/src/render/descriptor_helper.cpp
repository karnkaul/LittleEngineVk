#include <levk/core/utils/enumerate.hpp>
#include <levk/core/utils/expect.hpp>
#include <levk/engine/assets/asset_store.hpp>
#include <levk/engine/render/descriptor_helper.hpp>

namespace le {
DescriptorUpdater::DescriptorUpdater(not_null<Cache const*> cache, not_null<ShaderInput*> input, not_null<DescriptorBindings*> bindings, u32 setNumber,
									 std::size_t index)
	: m_cache(cache), m_input(input), m_vram(input->m_vram), m_bindings(bindings), m_index(index), m_setNumber(setNumber) {}

bool DescriptorUpdater::update(u32 bind, ShaderBuffer const& buffer) {
	if (check(bind)) {
		m_input->update(buffer, m_setNumber, bind, m_index);
		m_bindings->indices[m_setNumber] = m_index;
		return true;
	}
	return false;
}

bool DescriptorUpdater::update(u32 bind, Opt<Texture const> tex, TextureFallback fb) {
	if (check(bind)) {
		m_input->set(m_setNumber, m_index).update(bind, safeTex(tex, bind, fb));
		m_bindings->indices[m_setNumber] = m_index;
		return true;
	}
	return false;
}

bool DescriptorUpdater::check(u32 bind, vk::DescriptorType const* type, Texture::Type const* texType) noexcept {
	if (!valid()) { return false; }
	for (u32 const b : m_binds) {
		if (bind == b) { return true; }
	}
	if (m_input->contains(m_setNumber, bind, type, texType)) {
		m_binds.push_back(bind);
		return true;
	}
	return false;
}

graphics::Texture const& DescriptorUpdater::safeTex(Texture const* tex, u32 bind, TextureFallback fb) const {
	auto const texType = m_input->textureType(m_setNumber, bind);
	if (tex && tex->ready() && tex->type() == texType) { return *tex; }
	if (texType == Texture::Type::eCube) { return *m_cache->cube; }
	return *m_cache->defaults[fb];
}

DescriptorUpdater DescriptorMap::nextSet(DescriptorBindings& bindings, u32 setNumber) {
	return contains(setNumber) ? DescriptorUpdater(m_cache, m_input, &bindings, setNumber, m_nextIndex[setNumber]++) : DescriptorUpdater();
}

void DescriptorBinder::bind(DescriptorBindings const& indices) const {
	for (auto const [di, set] : utils::enumerate(indices.indices)) {
		if (di && m_input->contains(set)) { m_cb.bindSet(m_layout, m_input->set(set, *di)); }
	}
}

bool DescriptorBinder::bindNext(u32 setNumber) {
	if (m_input->contains(setNumber)) {
		m_cb.bindSet(m_layout, m_input->set(setNumber, m_nextIndex[setNumber]++));
		return true;
	}
	return false;
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
