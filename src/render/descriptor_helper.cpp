#include <core/utils/expect.hpp>
#include <engine/assets/asset_store.hpp>
#include <engine/render/descriptor_helper.hpp>

namespace le {
DescriptorUpdater::DescriptorUpdater(not_null<Cache const*> cache, not_null<ShaderInput*> input, u32 setNumber, std::size_t index)
	: m_cache(cache), m_input(input), m_vram(input->m_vram), m_index(index), m_setNumber(setNumber) {}

bool DescriptorUpdater::update(u32 bind, ShaderBuffer const& buffer) {
	if (check(bind)) {
		m_input->update(buffer, m_setNumber, bind, m_index);
		return true;
	}
	return false;
}

bool DescriptorUpdater::update(u32 bind, Texture const& tex) {
	if (check(bind)) {
		m_input->set(m_setNumber, m_index).update(bind, tex);
		return true;
	}
	return false;
}

bool DescriptorUpdater::update(u32 bind, Texture const* tex, TexBind tb) {
	if (check(bind)) {
		m_input->set(m_setNumber, m_index).update(bind, tex && tex->ready() ? *tex : *m_cache->defaults[tb]);
		return true;
	}
	return false;
}

bool DescriptorUpdater::check(u32 bind, vk::DescriptorType const* type) noexcept {
	if (!valid()) { return false; }
	for (u32 const b : m_binds) {
		if (bind == b) { return true; }
	}
	if (m_input->contains(m_setNumber, bind, type)) {
		m_binds.push_back(bind);
		return true;
	}
	return false;
}

DescriptorUpdater DescriptorMap::set(u32 setNumber) {
	return contains(setNumber) ? DescriptorUpdater(m_cache, m_input, setNumber, m_meta[setNumber]++) : DescriptorUpdater();
}

void DescriptorBinder::operator()(u32 set) {
	if (m_input->contains(set)) {
		auto const& s = m_input->set(set, m_meta[set]++);
		m_cb.bindSets(m_layout, s.get(), s.setNumber());
	}
}

DescriptorHelper::Cache DescriptorHelper::Cache::make(not_null<const AssetStore*> store) {
	Cache ret;
	ret.defaults[TexBind::eBlack] = store->find<Texture>("textures/black").peek();
	ret.defaults[TexBind::eWhite] = store->find<Texture>("textures/white").peek();
	ret.defaults[TexBind::eMagenta] = store->find<Texture>("textures/magenta").peek();
	EXPECT(ret.defaults[TexBind::eBlack] && ret.defaults[TexBind::eWhite] && ret.defaults[TexBind::eMagenta]);
	return ret;
}
} // namespace le
