#include <graphics/context/device.hpp>
#include <graphics/context/queues.hpp>

namespace le::graphics {
void Queues::setup(vk::Device device, EnumArray<QType, Indices> data) {
	for (std::size_t idx = 0; idx < data.size(); ++idx) {
		Queue& queue = m_queues[idx];
		Indices& d = data[idx];
		queue.arrayIndex = d.arrayIndex;
		queue.familyIndex = d.familyIndex;
		queue.queue = device.getQueue(queue.familyIndex, queue.arrayIndex);
	}
}

std::vector<u32> Queues::familyIndices(QFlags flags) const {
	std::vector<u32> ret;
	ret.reserve(3);
	if (flags.test(QType::eGraphics)) {
		ret.push_back(queue(QType::eGraphics).familyIndex);
	}
	if (flags.test(QType::ePresent) && queue(QType::ePresent).familyIndex != queue(QType::eGraphics).familyIndex) {
		ret.push_back(queue(QType::ePresent).familyIndex);
	}
	if (flags.test(QType::eTransfer) && queue(QType::eTransfer).familyIndex != queue(QType::eGraphics).familyIndex) {
		ret.push_back(queue(QType::eTransfer).familyIndex);
	}
	return ret;
}

vk::Result Queues::present(vk::PresentInfoKHR const& info) {
	return queue(QType::ePresent).queue.presentKHR(info);
}

void Queues::waitIdle(QType type) const {
	queue(type).queue.waitIdle();
}

void Queues::submit(QType type, vAP<vk::SubmitInfo> infos, vk::Fence signal) {
	auto lock = m_mutex.lock();
	vk::Queue queue = m_queues[(std::size_t)type].queue;
	queue.submit(infos, signal);
}

} // namespace le::graphics
