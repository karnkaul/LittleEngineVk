#include <graphics/common.hpp>
#include <graphics/context/device.hpp>
#include <graphics/shader.hpp>

namespace le::graphics {
Shader::Shader(not_null<Device*> device, std::string name) : m_name(std::move(name)), m_device(device) {}

Shader::Shader(not_null<Device*> device, std::string name, SpirVMap const& spirV) : m_name(std::move(name)), m_device(device) {
	construct(spirV, m_spirV, m_modules);
}

Shader::Shader(Shader&& rhs) : m_name(std::move(rhs.m_name)), m_modules(std::move(rhs.m_modules)), m_spirV(std::move(rhs.m_spirV)), m_device(rhs.m_device) {
	for (auto& m : rhs.m_modules) {
		m = vk::ShaderModule();
	}
}

Shader& Shader::operator=(Shader&& rhs) {
	if (&rhs != this) {
		destroy();
		m_name = std::move(rhs.m_name);
		m_modules = std::move(rhs.m_modules);
		m_spirV = std::move(rhs.m_spirV);
		m_device = rhs.m_device;
		for (auto& m : rhs.m_modules) {
			m = vk::ShaderModule();
		}
	}
	return *this;
}

Shader::~Shader() { destroy(); }

bool Shader::reconstruct(SpirVMap const& spirV) {
	CodeMap code;
	ModuleMap map;
	if (construct(spirV, code, map)) {
		destroy();
		m_spirV = std::move(code);
		m_modules = std::move(map);
		return true;
	}
	return false;
}

bool Shader::construct(SpirVMap const& spirV, CodeMap& out_code, ModuleMap& out_map) {
	if (std::all_of(spirV.begin(), spirV.end(), [](auto const& v) -> bool { return v.empty(); })) {
		return false;
	}
	for (std::size_t idx = 0; idx < spirV.size(); ++idx) {
		ENSURE(spirV[idx].size() % 4 == 0, "Invalid SPIR-V");
		out_code[idx].clear();
		out_code[idx].resize(spirV[idx].size() / 4);
		std::memcpy(out_code[idx].data(), spirV[idx].data(), spirV[idx].size());
	}
	std::size_t idx = 0;
	for (auto const& code : out_code) {
		if (!code.empty()) {
			vk::ShaderModuleCreateInfo createInfo;
			createInfo.codeSize = code.size() * sizeof(decltype(code[0]));
			createInfo.pCode = code.data();
			out_map[idx++] = m_device->device().createShaderModule(createInfo);
		}
	}
	return true;
}

vk::ShaderStageFlags Shader::stages() const noexcept {
	vk::ShaderStageFlags ret;
	for (std::size_t idx = 0; idx < m_modules.size(); ++idx) {
		if (!Device::default_v(m_modules[idx])) {
			ret |= typeToFlag[idx];
		}
	}
	return ret;
}

bool Shader::valid() const noexcept { return stages() != vk::ShaderStageFlags(); }

bool Shader::empty() const noexcept { return !valid(); }

void Shader::destroy() {
	for (auto& module : m_modules) {
		if (!Device::default_v(module)) {
			m_device->device().destroyShaderModule(module);
		}
		module = vk::ShaderModule();
	}
}
} // namespace le::graphics
