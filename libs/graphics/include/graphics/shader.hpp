#pragma once
#include <graphics/types.hpp>

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

	Shader(Device& device);
	Shader(Device& device, SpirVMap bytes);
	Shader(Shader&&);
	Shader& operator=(Shader&&);
	~Shader();

	bool reconstruct(SpirVMap spirV);

	vk::ShaderStageFlags stages() const noexcept;
	bool valid() const noexcept;
	bool empty() const noexcept;

	ModuleMap m_modules;
	CodeMap m_spirV;

  protected:
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
