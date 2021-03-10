#pragma once
#include <core/ref.hpp>
#include <core/std_types.hpp>
#include <vulkan/vulkan.hpp>

namespace le::graphics {
class Device;

class Shader {
  public:
	enum class Type { eVertex, eFragment, eCOUNT_ };
	static constexpr std::array typeToFlag = {vk::ShaderStageFlagBits::eVertex, vk::ShaderStageFlagBits::eFragment};

	struct Resources;

	template <typename T>
	using ArrayMap = EnumArray<Type, T>;
	using SpirVMap = ArrayMap<bytearray>;
	using CodeMap = ArrayMap<std::vector<u32>>;
	using ModuleMap = ArrayMap<vk::ShaderModule>;
	using ResourcesMap = ArrayMap<Resources>;

	Shader(Device& device, std::string name);
	Shader(Device& device, std::string name, SpirVMap const& bytes);
	Shader(Shader&&);
	Shader& operator=(Shader&&);
	~Shader();

	bool reconstruct(SpirVMap const& spirV);

	vk::ShaderStageFlags stages() const noexcept;
	bool valid() const noexcept;
	bool empty() const noexcept;

	std::string m_name;
	ModuleMap m_modules;
	CodeMap m_spirV;

  protected:
	bool construct(SpirVMap const& spirV, CodeMap& out_code, ModuleMap& out_map);
	void destroy();

	Ref<Device> m_device;
};

constexpr bool operator==(Shader const& lhs, Shader const& rhs) noexcept {
	for (std::size_t idx = 0; idx < lhs.m_modules.size(); ++idx) {
		if (lhs.m_modules[idx] != rhs.m_modules[idx]) {
			return false;
		}
	}
	return true;
}

constexpr bool operator!=(Shader const& lhs, Shader const& rhs) noexcept {
	return !(lhs == rhs);
}
} // namespace le::graphics
