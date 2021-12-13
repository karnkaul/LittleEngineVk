#include <core/log_channel.hpp>
#include <core/utils/error.hpp>
#include <core/utils/expect.hpp>
#include <graphics/common.hpp>
#include <graphics/context/device.hpp>
#include <graphics/context/queue.hpp>
#include <map>
#include <set>

namespace le::graphics {
void Queue::setup(vk::Queue queue, u32 family, QCaps qcaps) {
	m_family = family;
	m_queue.t = queue;
	m_qcaps = qcaps;
}

vk::CommandPool Queue::makeCommandPool(vk::Device device, vk::CommandPoolCreateFlags flags) const {
	if (valid()) {
		vk::CommandPoolCreateInfo info(flags, m_family);
		return device.createCommandPool(info);
	}
	return {};
}

vk::Result Queue::present(vk::PresentInfoKHR const& info) const { return valid() ? ktl::tlock(m_queue)->presentKHR(&info) : vk::Result::eErrorDeviceLost; }

vk::Result Queue::submit(vAP<vk::SubmitInfo> infos, vk::Fence signal) const {
	if (valid()) {
		ktl::tlock(m_queue)->submit(infos, signal);
		return vk::Result::eSuccess;
	}
	return vk::Result::eErrorDeviceLost;
}

bool Queues::hasCompute() const noexcept { return m_primary.capabilities().test(QType::eCompute) || m_secondary.capabilities().test(QType::eCompute); }

Queue const* Queues::compute() const noexcept {
	if (m_primary.capabilities().test(QType::eCompute)) { return &m_primary; }
	if (m_secondary.capabilities().test(QType::eCompute)) { return &m_secondary; }
	return {};
}

ktl::fixed_vector<vk::DeviceQueueCreateInfo, 2> Queues::Selector::select(Span<Info const> infos) {
	static f32 const priority = 1.0f;
	ktl::fixed_vector<vk::DeviceQueueCreateInfo, 2> ret;
	auto const gpt = QFlags(QFlag::eGraphics) | QFlag::eTransfer | QFlag::ePresent;
	for (Info const& info : infos) {
		if (info.flags.all(gpt)) {
			ret.push_back(vk::DeviceQueueCreateInfo({}, info.family, 1U, &priority));
			m_primary.family = info.family;
			m_primary.qcaps = QType::eGraphics;
			if (info.flags.test(QFlag::eCompute)) { m_primary.qcaps.set(QType::eCompute); }
			break;
		}
	}
	EXPECT(m_primary.qcaps.test(QType::eGraphics));
	if (!m_primary.qcaps.test(QType::eCompute)) {
		for (Info const& info : infos) {
			if (info.flags.all(QFlag::eCompute)) {
				ret.push_back(vk::DeviceQueueCreateInfo({}, info.family, 1U, &priority));
				m_secondary.family = info.family;
				m_secondary.qcaps = QType::eCompute;
				break;
			}
		}
	}
	return ret;
}

void Queues::Selector::setup(vk::Device device) {
	EXPECT(m_primary.qcaps.any());
	m_queues.m_primary.setup(device.getQueue(m_primary.family, 0U), m_primary.family, m_primary.qcaps);
	if (m_secondary.qcaps.any()) {
		m_queues.m_secondary.setup(device.getQueue(m_secondary.family, 0U), m_secondary.family, m_secondary.qcaps);
		logI(LC_LibUser, "[{}] Multiplexing [2] Vulkan queue families [{}, {}] for [Graphics, Compute]", g_name, m_primary.family, m_secondary.family);
	} else {
		std::string_view const types = m_primary.qcaps.test(QType::eCompute) ? "Graphics/Compute" : "Graphics";
		logI(LC_LibUser, "[{}] Multiplexing [1] Vulkan queue family [{}] for [{}]", g_name, m_primary.family, types);
	}
}
} // namespace le::graphics
