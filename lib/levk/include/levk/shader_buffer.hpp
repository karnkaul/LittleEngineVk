#pragma once
#include <levk/core/polymorphic.hpp>
#include <levk/descriptor_info.hpp>
#include <cstddef>
#include <vector>

namespace levk {
/// \brief Abstract base for uniform / storage buffers.
class IShaderBuffer : public Polymorphic {
  public:
	struct View {
		vk::Buffer buffer{};
		vk::DeviceSize size{};
		vk::DescriptorType type{};
	};

	[[nodiscard]] virtual auto get_view() const -> View const& = 0;
	[[nodiscard]] virtual auto get_descriptor_info(std::uint32_t binding) const -> DescriptorInfo = 0;

	virtual void set_data(std::vector<std::byte> bytes) = 0;

	void set_data(void const* data, std::size_t const size) {
		auto vec = std::vector<std::byte>(size);
		std::memcpy(vec.data(), data, size);
		set_data(std::move(vec));
	}
};

/// \brief Opaque interface for uniform buffer.
class IUniformBuffer : public IShaderBuffer {};

/// \brief Opaque interface for storage buffer.
class IStorageBuffer : public IShaderBuffer {};
} // namespace levk
