#pragma once
#include <core/span.hpp>
#include <core/std_types.hpp>
#include <graphics/qflags.hpp>
#include <ktl/fixed_vector.hpp>
#include <ktl/tmutex.hpp>
#include <vulkan/vulkan.hpp>

namespace le::graphics {
class Queue : public Pinned {
  public:
	enum class Cap { eNone, eGraphics, eCompute };
	using Caps = ktl::enum_flags<Cap, u8>;

	void setup(vk::Queue queue, u32 family, Caps caps);

	bool valid() const noexcept { return capabilities().any(); }
	u32 family() const noexcept { return m_family; }
	Caps capabilities() const noexcept { return m_caps; }
	vk::Queue queue() const noexcept { return m_queue.t; }
	explicit operator bool() const noexcept { return valid(); }

	vk::CommandPool makeCommandPool(vk::Device device, vk::CommandPoolCreateFlags flags) const;

	vk::Result submit(vk::ArrayProxy<vk::SubmitInfo const> const& infos, vk::Fence signal) const;
	vk::Result present(vk::PresentInfoKHR const& info) const;

  private:
	ktl::tmutex<vk::Queue> m_queue;
	u32 m_family = 0;
	Caps m_caps;
};

class Queues : public Pinned {
  public:
	using QType = Queue::Cap;

	struct Info {
		u32 family = 0;
		QFlags flags;
	};

	class Selector;

	bool hasCompute() const noexcept;
	Queue const& primary() const noexcept { return m_primary; }
	Queue const& secondary() const noexcept { return m_secondary; }
	Queue const* compute() const noexcept;

  private:
	Queue m_primary;
	Queue m_secondary;
};

class Queues::Selector {
  public:
	Selector(Queues& queues) noexcept : m_queues(queues) {}

	ktl::fixed_vector<vk::DeviceQueueCreateInfo, 2> select(Span<Info const> infos);
	void setup(vk::Device device);

  private:
	using Caps = Queue::Caps;

	struct Select {
		u32 family{};
		Caps caps;
	};

	Queues& m_queues;
	Select m_primary;
	Select m_secondary;
};
} // namespace le::graphics
