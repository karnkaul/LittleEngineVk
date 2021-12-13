#pragma once
#include <core/span.hpp>
#include <core/std_types.hpp>
#include <graphics/qtype.hpp>
#include <ktl/tmutex.hpp>
#include <vulkan/vulkan.hpp>

namespace le::graphics {
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
	ktl::tmutex<vk::Queue> m_queue;
	u32 m_family = 0;
	QCaps m_qcaps;
};

class Queues : public Pinned {
  public:
	enum class QFlag { eGraphics, ePresent, eTransfer, eCompute, eCOUNT_ };
	using QFlags = ktl::enum_flags<QFlag, u8>;

	struct Info;
	class Selector;

	bool hasCompute() const noexcept;
	Queue const& primary() const noexcept { return m_primary; }
	Queue const& secondary() const noexcept { return m_secondary; }
	Queue const* compute() const noexcept;

  private:
	Queue m_primary;
	Queue m_secondary;
};

struct Queues::Info {
	u32 family = 0;
	QFlags flags;
};

class Queues::Selector {
  public:
	Selector(Queues& queues) noexcept : m_queues(queues) {}

	ktl::fixed_vector<vk::DeviceQueueCreateInfo, 2> select(Span<Info const> infos);
	void setup(vk::Device device);

  private:
	struct Select {
		u32 family{};
		QCaps qcaps;
	};

	Queues& m_queues;
	Select m_primary;
	Select m_secondary;
};
} // namespace le::graphics
