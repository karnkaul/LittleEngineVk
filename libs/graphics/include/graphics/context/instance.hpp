#pragma once
#include <memory>
#include <core/log.hpp>
#include <core/span.hpp>
#include <graphics/context/physical_device.hpp>
#include <ktl/fixed_vector.hpp>

namespace le::graphics {
enum class Validation { eOn, eOff };

class Instance final {
  public:
	struct CreateInfo;

	Instance() = default;
	Instance(CreateInfo const& info);
	Instance(Instance&&);
	Instance& operator=(Instance&&);
	~Instance();

	ktl::fixed_vector<PhysicalDevice, 8> availableDevices(Span<std::string_view const> requiredExtensions) const;
	vk::Instance instance() const noexcept { return m_instance; }
	vk::DispatchLoaderDynamic loader() const { return m_loader; }

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
	struct {
		dl::level logLevel = dl::level::info;
		Validation mode = Validation::eOff;
	} validation;
};
} // namespace le::graphics
