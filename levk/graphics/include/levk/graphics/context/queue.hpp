#pragma once
#include <ktl/async/kmutex.hpp>
#include <levk/core/span.hpp>
#include <levk/core/std_types.hpp>
#include <levk/graphics/qtype.hpp>
#include <vulkan/vulkan.hpp>

namespace le::graphics {
struct PhysicalDevice;

class Queue : public Pinned {
  public:
	void setup(vk::Queue queue, u32 family, QCaps qcaps);

	bool valid() const noexcept { return capabilities().any(); }
	u32 family() const noexcept { return m_family; }
	QCaps capabilities() const noexcept { return m_qcaps; }
	vk::Queue queue() const noexcept { return m_queue.t; }
	explicit operator bool() const noexcept { return valid(); }

	vk::CommandPool makeCommandPool(vk::Device device, vk::CommandPoolCreateFlags flags) const;

	vk::Result submit(vk::ArrayProxy<vk::SubmitInfo const> const& infos, vk::Fence signal) const;
	vk::Result present(vk::PresentInfoKHR const& info) const;

  private:
	ktl::kmutex<vk::Queue> m_queue;
	u32 m_family = 0;
	QCaps m_qcaps;
};

class Queues : public Pinned {
  public:
	enum class QFlag { eGraphics, ePresent, eTransfer, eCompute, eCOUNT_ };
	using QFlags = ktl::enum_flags<QFlag, u8>;

	struct Info;
	struct Select;

	static Select select(PhysicalDevice const& device, vk::SurfaceKHR surface);
	bool setup(vk::Device device, Select const& select);

	Queue const& primary() const noexcept { return m_primary; }
	Queue const& secondary() const noexcept { return m_secondary; }

	bool hasCompute() const noexcept;
	Queue const& graphics() const noexcept { return primary(); }
	Queue const& transfer() const noexcept { return primary(); }
	Queue const* compute() const noexcept;

  private:
	Queue m_primary;
	Queue m_secondary;
};

struct Queues::Info {
	u32 family = 0;
	QCaps qcaps;
};

struct Queues::Select {
	ktl::fixed_vector<vk::DeviceQueueCreateInfo, 2> dqci;
	ktl::fixed_vector<Info, 2> info;
	std::size_t primary{};
};
} // namespace le::graphics
