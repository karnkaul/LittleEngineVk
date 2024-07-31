#include <levk/render_device.hpp>
#include <levk/shader_buffer.hpp>

namespace levk {
template <std::derived_from<IShaderBuffer> Base>
class ShaderBuffer : public Base {
  public:
	using View = IShaderBuffer::View;

	static constexpr auto usage_v = std::same_as<Base, IStorageBuffer> ? vk::BufferUsageFlagBits::eStorageBuffer : vk::BufferUsageFlagBits::eUniformBuffer;
	static constexpr auto type_v = usage_v == vk::BufferUsageFlagBits::eStorageBuffer ? vk::DescriptorType::eStorageBuffer : vk::DescriptorType::eUniformBuffer;

	static constexpr auto get_name() -> CString {
		if constexpr (type_v == vk::DescriptorType::eStorageBuffer) {
			return "ssbo";
		} else {
			return "ubo";
		}
	}

	explicit ShaderBuffer(NotNull<IRenderDevice*> device, vk::DeviceSize const size) : m_device(device) {
		auto const bci = BufferCreateInfo{
			.usage = usage_v,
			.size = size,
			.map_memory = true,
		};
		for (auto& storage : m_storage) {
			storage.buffer = device->create_buffer(bci);
			storage.buffer->set_name(get_name());
			storage.dirty = true;
		}
	}

	auto get_view() const -> View const& final {
		refresh();
		auto& storage = m_storage.at(m_device->get_frame_index());
		return storage.view;
	}

	auto get_descriptor_info(std::uint32_t const binding) const -> DescriptorInfo final {
		refresh();
		auto& storage = m_storage.at(m_device->get_frame_index());
		if (storage.view.size == 0) { return {}; }

		return DescriptorInfo{
			.payload = vk::DescriptorBufferInfo{storage.buffer->get_buffer_info().buffer, vk::DeviceSize{}, storage.view.size},
			.type = type_v,
			.binding = binding,
		};
	}

	void set_data(std::vector<std::byte> bytes) final {
		m_data = std::move(bytes);
		for (auto& storage : m_storage) { storage.dirty = true; }
	}

  private:
	struct Storage {
		std::unique_ptr<IRenderBuffer> buffer{};
		View view{};
		bool dirty{};
	};

	void refresh() const {
		auto& storage = m_storage.at(m_device->get_frame_index());
		if (!storage.dirty) { return; }
		storage.view = write_data(*storage.buffer);
		storage.dirty = false;
	}

	[[nodiscard]] auto write_data(IRenderBuffer& out) const -> View {
		out.write_data({m_data.data(), m_data.size()});
		return View{
			.buffer = out.get_buffer_info().buffer,
			.size = m_data.size(),
			.type = type_v,
		};
	}

	NotNull<IRenderDevice*> m_device;

	mutable Buffered<Storage> m_storage{};
	std::vector<std::byte> m_data{};
};
} // namespace levk
