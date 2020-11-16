#include <graphics/context/device.hpp>
#include <graphics/shader.hpp>

namespace le::graphics {
Shader::Shader(Device& device) : m_device(device) {
}

Shader::Shader(Device& device, SpirVMap spirV) : m_device(device) {
	reconstruct(std::move(spirV));
}

Shader::Shader(Shader&& rhs) : m_modules(std::move(rhs.m_modules)), m_spirV(std::move(rhs.m_spirV)), m_device(rhs.m_device) {
	for (auto& m : rhs.m_modules) {
		m = vk::ShaderModule();
	}
}

Shader& Shader::operator=(Shader&& rhs) {
	if (&rhs != this) {
		destroy();
		m_modules = std::move(rhs.m_modules);
		m_spirV = std::move(rhs.m_spirV);
		m_device = rhs.m_device;
		for (auto& m : rhs.m_modules) {
			m = vk::ShaderModule();
		}
	}
	return *this;
}

Shader::~Shader() {
	destroy();
}

bool Shader::reconstruct(SpirVMap spirV) {
	destroy();
	if (std::all_of(spirV.begin(), spirV.end(), [](auto const& v) -> bool { return v.empty(); })) {
		return false;
	}
	for (std::size_t idx = 0; idx < spirV.size(); ++idx) {
		ENSURE(spirV[idx].size() % 4 == 0, "Invalid SPIR-V");
		m_spirV[idx].clear();
		m_spirV[idx].resize(spirV[idx].size() / 4);
		std::memcpy(m_spirV[idx].data(), spirV[idx].data(), spirV[idx].size());
	}
	std::size_t idx = 0;
	Device& d = m_device;
	for (auto const& code : m_spirV) {
		if (!code.empty()) {
			vk::ShaderModuleCreateInfo createInfo;
			createInfo.codeSize = code.size() * sizeof(decltype(code[0]));
			createInfo.pCode = code.data();
			m_modules[idx++] = d.m_device.createShaderModule(createInfo);
		}
	}
	return valid();
}

vk::ShaderStageFlags Shader::stages() const noexcept {
	vk::ShaderStageFlags ret;
	for (std::size_t idx = 0; idx < m_modules.size(); ++idx) {
		if (!default_v(m_modules[idx])) {
			ret |= typeToFlag[idx];
		}
	}
	return ret;
}

bool Shader::valid() const noexcept {
	return stages() != vk::ShaderStageFlags();
}

bool Shader::empty() const noexcept {
	return !valid();
}

void Shader::destroy() {
	Device& d = m_device;
	for (auto module : m_modules) {
		if (!default_v(module)) {
			d.m_device.destroyShaderModule(module);
		}
	}
}
} // namespace le::graphics
