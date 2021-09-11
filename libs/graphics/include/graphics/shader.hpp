#pragma once
#include <core/not_null.hpp>
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

	Shader(not_null<Device*> device, std::string name);
	Shader(not_null<Device*> device, std::string name, SpirVMap const& bytes);
	Shader(Shader&& rhs) noexcept : Shader(rhs.m_device) { exchg(*this, rhs); }
	Shader& operator=(Shader rhs) noexcept { return (exchg(*this, rhs), *this); }
	~Shader();
	static void exchg(Shader& lhs, Shader& rhs) noexcept;

	bool reconstruct(SpirVMap const& spirV);

	vk::ShaderStageFlags stages() const noexcept;
	bool valid() const noexcept;
	bool empty() const noexcept;

	std::string m_name;
	ModuleMap m_modules = {};
	CodeMap m_spirV = {};

  private:
	Shader(not_null<Device*> device) noexcept : m_device(device) {}

	bool construct(SpirVMap const& spirV, CodeMap& out_code, ModuleMap& out_map);
	void destroy();

	not_null<Device*> m_device;
};

constexpr bool operator==(Shader const& lhs, Shader const& rhs) noexcept {
	for (std::size_t idx = 0; idx < arraySize(lhs.m_modules.arr); ++idx) {
		if (lhs.m_modules.arr[idx] != rhs.m_modules.arr[idx]) { return false; }
	}
	return true;
}

constexpr bool operator!=(Shader const& lhs, Shader const& rhs) noexcept { return !(lhs == rhs); }
} // namespace le::graphics
