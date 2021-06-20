#pragma once
#include <memory>
#include <core/log.hpp>
#include <core/span.hpp>
#include <graphics/context/physical_device.hpp>
#include <kt/fixed_vector/fixed_vector.hpp>

namespace le::graphics {
class Instance final {
  public:
	struct CreateInfo;

	Instance() = default;
	Instance(CreateInfo const& info);
	Instance(Instance&&);
	Instance& operator=(Instance&&);
	~Instance();

	kt::fixed_vector<PhysicalDevice, 8> availableDevices(Span<std::string_view const> requiredExtensions) const;
	vk::Instance instance() const noexcept { return m_instance; }
	vk::DispatchLoaderDynamic loader() const { return m_loader; }

	inline static std::optional<bool> s_forceValidation;

  private:
	struct {
		std::vector<char const*> extensions;
		std::vector<char const*> layers;
	} m_metadata;

	vk::Instance m_instance;
	vk::DispatchLoaderDynamic m_loader;
	vk::DebugUtilsMessengerEXT m_messenger;

	void destroy();

	friend class Device;
};

struct Instance::CreateInfo {
	Span<std::string_view const> extensions;
	dl::level validationLog = dl::level::info;
	bool bValidation = false;
};
} // namespace le::graphics
