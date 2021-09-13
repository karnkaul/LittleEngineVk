#include <graphics/common.hpp>
#include <graphics/context/device.hpp>
#include <graphics/shader.hpp>

namespace le::graphics {
Shader::Shader(not_null<Device*> device, std::string name) : m_name(std::move(name)), m_device(device) {}

Shader::Shader(not_null<Device*> device, std::string name, SpirVMap const& spirV) : m_name(std::move(name)), m_device(device) {
	construct(spirV, m_spirV, m_modules);
}

void Shader::exchg(Shader& lhs, Shader& rhs) noexcept {
	std::swap(lhs.m_name, rhs.m_name);
	std::swap(lhs.m_modules, rhs.m_modules);
	std::swap(lhs.m_spirV, rhs.m_spirV);
	std::swap(lhs.m_device, rhs.m_device);
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
	if (std::all_of(std::begin(spirV.arr), std::end(spirV.arr), [](auto const& v) { return v.empty(); })) { return false; }
	for (std::size_t idx = 0; idx < arraySize(spirV.arr); ++idx) {
		ensure(spirV.arr[idx].size() % 4 == 0, "Invalid SPIR-V");
		out_code.arr[idx].clear();
		out_code.arr[idx].resize(spirV.arr[idx].size() / 4);
		std::memcpy(out_code.arr[idx].data(), spirV.arr[idx].data(), spirV.arr[idx].size());
	}
	std::size_t idx = 0;
	for (auto const& code : out_code.arr) {
		if (!code.empty()) {
			vk::ShaderModuleCreateInfo createInfo;
			createInfo.codeSize = code.size() * sizeof(decltype(code[0]));
			createInfo.pCode = code.data();
			out_map.arr[idx++] = m_device->device().createShaderModule(createInfo);
		}
	}
	return true;
}

vk::ShaderStageFlags Shader::stages() const noexcept {
	vk::ShaderStageFlags ret;
	for (std::size_t idx = 0; idx < arraySize(m_modules.arr); ++idx) {
		if (!Device::default_v(m_modules.arr[idx])) { ret |= typeToFlag[idx]; }
	}
	return ret;
}

bool Shader::valid() const noexcept { return stages() != vk::ShaderStageFlags(); }

bool Shader::empty() const noexcept { return !valid(); }

void Shader::destroy() {
	for (auto& module : m_modules.arr) {
		if (!Device::default_v(module)) { m_device->device().destroyShaderModule(module); }
		module = vk::ShaderModule();
	}
}
} // namespace le::graphics
