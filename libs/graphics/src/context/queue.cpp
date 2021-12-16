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

bool Queues::setup(vk::Device device, Select const& select) {
	EXPECT(!select.info.empty());
	if (select.info.empty()) { return false; }
	auto const& primary = select.info.front().qcaps.test(QType::eGraphics) ? select.info.front() : select.info.back();
	EXPECT(primary.qcaps.test(QType::eGraphics));
	if (!primary.qcaps.test(QType::eGraphics)) { return false; }
	m_primary.setup(device.getQueue(primary.family, 0U), primary.family, primary.qcaps);
	if (select.info.size() > 1U) {
		auto const& secondary = primary.family == select.info.front().family ? select.info.back() : select.info.front();
		EXPECT(secondary.family != primary.family);
		m_secondary.setup(device.getQueue(secondary.family, 0U), secondary.family, secondary.qcaps);
		logI(LC_LibUser, "[{}] Multiplexing [2] Vulkan queue families [{}, {}] for [Graphics, Compute]", g_name, primary.family, secondary.family);
		return true;
	}
	std::string_view const types = primary.qcaps.test(QType::eCompute) ? "Graphics/Compute" : "Graphics";
	logI(LC_LibUser, "[{}] Multiplexing [1] Vulkan queue family [{}] for [{}]", g_name, primary.family, types);
	return true;
}

bool Queues::hasCompute() const noexcept { return m_primary.capabilities().test(QType::eCompute) || m_secondary.capabilities().test(QType::eCompute); }

Queue const* Queues::compute() const noexcept {
	if (m_primary.capabilities().test(QType::eCompute)) { return &m_primary; }
	if (m_secondary.capabilities().test(QType::eCompute)) { return &m_secondary; }
	return {};
}

Queues::Select Queues::select(PhysicalDevice const& device, vk::SurfaceKHR surface) {
	using vQFB = vk::QueueFlagBits;
	Select ret;
	u32 family{};
	static f32 const priority = 1.0f;
	bool gptFound = false;
	for (vk::QueueFamilyProperties const& props : device.queueFamilies) {
		bool const presentable = device.surfaceSupport(family, surface);
		if ((ret.info.empty() || !ret.info.front().qcaps.test(QType::eGraphics)) && props.queueFlags & vQFB::eGraphics && presentable) {
			QCaps caps = QType::eGraphics;
			gptFound = true;
			if (props.queueFlags & vQFB::eCompute) {
				caps |= QType::eCompute;
				ret.info.push_back({family, caps});
				ret.dqci.push_back(vk::DeviceQueueCreateInfo({}, family, 1U, &priority));
				return ret;
			}
			ret.info.push_back({family, caps});
			ret.dqci.push_back(vk::DeviceQueueCreateInfo({}, family, 1U, &priority));
		}
		if ((ret.info.empty() || !ret.info.front().qcaps.test(QType::eCompute)) && props.queueFlags & vQFB::eCompute) {
			QCaps caps = QType::eCompute;
			ret.info.push_back({family, caps});
			ret.dqci.push_back(vk::DeviceQueueCreateInfo({}, family, 1U, &priority));
			return ret;
		}
		++family;
	}
	EXPECT(gptFound);
	return ret;
}
} // namespace le::graphics
