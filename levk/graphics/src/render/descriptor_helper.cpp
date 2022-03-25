#include <ktl/enumerate.hpp>
#include <levk/core/utils/expect.hpp>
#include <levk/graphics/render/descriptor_helper.hpp>
#include <levk/graphics/render/pipeline_factory.hpp>
#include <levk/graphics/render/shader_buffer.hpp>

namespace le::graphics {
bool DescriptorUpdater::update(u32 binding, ShaderBuffer const& buffer) const {
	if (check(binding)) {
		buffer.update(*m_descriptorSet, binding);
		return true;
	}
	return false;
}

bool DescriptorUpdater::update(u32 binding, Opt<Texture const> tex) const {
	if (check(binding)) {
		m_descriptorSet->update(binding, safeTex(tex, binding));
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

Texture const& DescriptorUpdater::safeTex(Texture const* tex, u32 bind) const {
	auto const texType = m_descriptorSet->textureType(bind);
	if (tex && tex->ready() && tex->type() == texType) { return *tex; }
	if (texType == Texture::Type::eCube) { return *m_fallback.cubemap; }
	return *m_fallback.texture;
}

bool DescriptorMap::contains(u32 setNumber) { return m_input->contains(setNumber); }

DescriptorUpdater DescriptorMap::nextSet(DrawBindings const& bindings, u32 setNumber) {
	if (contains(setNumber)) {
		auto const index = m_nextIndex[setNumber]++;
		bindings.bind(setNumber, index);
		return DescriptorUpdater(m_fallback, &m_input->set(setNumber, index));
	}
	return DescriptorUpdater(m_fallback);
}

void DescriptorBinder::bind(DrawBindings const& indices) const {
	for (auto const [di, set] : ktl::enumerate<u32>(indices.indices)) {
		if (di && m_input->contains((u32)set)) { m_cb.bindSet(m_layout, m_input->set(set, *di)); }
	}
}
} // namespace le::graphics
