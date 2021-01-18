#include <graphics/common.hpp>
#include <graphics/context/device.hpp>
#include <graphics/context/transfer.hpp>

namespace le::graphics {
namespace {
constexpr vk::DeviceSize ceilPOT(vk::DeviceSize size) noexcept {
	vk::DeviceSize ret = 2;
	while (ret < size) {
		ret <<= 1;
	}
	return ret;
}

Buffer createStagingBuffer(Memory& memory, vk::DeviceSize size) {
	Buffer::CreateInfo info;
	info.size = ceilPOT(size);
	info.properties = vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent;
	info.usage = vk::BufferUsageFlagBits::eTransferSrc;
	info.queueFlags = QType::eGraphics | QType::eTransfer;
	info.vmaUsage = VMA_MEMORY_USAGE_CPU_ONLY;
#if defined(LEVK_VKRESOURCE_NAMES)
	static u64 s_nextBufferID = 0;
	info.name = fmt::format("vram_staging/{}", s_nextBufferID++);
#endif
	return Buffer(memory, info);
}
} // namespace

Transfer::Transfer(Memory& memory, CreateInfo const& info) : m_memory(memory) {
	vk::CommandPoolCreateInfo poolInfo;
	poolInfo.flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer;
	poolInfo.queueFamilyIndex = memory.m_device.get().queues().familyIndex(QType::eTransfer);
	m_data.pool = memory.m_device.get().device().createCommandPool(poolInfo);
	m_queue.active(true);
	m_sync.stagingThread = threads::newThread([this, r = info.reserve]() {
		{
			auto lock = m_sync.mutex.lock();
			for (auto const& range : r) {
				for (auto i = range.count; i > 0; --i) {
					m_data.buffers.push_back(createStagingBuffer(m_memory, range.size));
				}
			}
		}
		g_log.log(lvl::info, 1, "[{}] Transfer thread started", g_name);
		while (auto f = m_queue.pop()) {
			(*f)();
		}
		g_log.log(lvl::info, 1, "[{}] Transfer thread completed", g_name);
	});
	if (info.autoPollRate && *info.autoPollRate > 0ms) {
		m_sync.bPoll.store(true);
		m_sync.pollThread = threads::newThread([this, rate = *info.autoPollRate]() {
			g_log.log(lvl::info, 1, "[{}] Transfer poll thread started", g_name);
			while (m_sync.bPoll.load()) {
				update();
				threads::sleep(rate);
			}
			g_log.log(lvl::info, 1, "[{}] Transfer poll thread completed", g_name);
		});
	}
	g_log.log(lvl::info, 1, "[{}] Transfer constructed", g_name);
}

Transfer::~Transfer() {
	m_sync.bPoll.store(false);
	auto residue = m_queue.clear();
	for (auto& f : residue) {
		f();
	}
	m_sync.stagingThread = {};
	Memory& m = m_memory;
	Device& d = m.m_device;
	d.waitIdle();
	d.destroy(m_data.pool);
	for (auto& fence : m_data.fences) {
		d.destroy(fence);
	}
	for (auto& batch : m_batches.submitted) {
		d.destroy(batch.done);
	}
	m_data = {};
	m_batches = {};
	d.waitIdle(); // force flush deferred
	g_log.log(lvl::info, 1, "[{}] Transfer destroyed", g_name);
}

std::size_t Transfer::update() {
	auto removeDone = [this](Batch& batch) -> bool {
		if (m_memory.get().m_device.get().signalled(batch.done)) {
			if (batch.framePad == 0) {
				for (auto& [stage, promise] : batch.entries) {
					promise->set_value();
					scavenge(std::move(stage), batch.done);
				}
				return true;
			}
			--batch.framePad;
		}
		return false;
	};
	auto lock = m_sync.mutex.lock();
	m_batches.submitted.erase(std::remove_if(m_batches.submitted.begin(), m_batches.submitted.end(), removeDone), m_batches.submitted.end());
	if (!m_batches.active.entries.empty()) {
		std::vector<vk::CommandBuffer> commands;
		commands.reserve(m_batches.active.entries.size());
		m_batches.active.done = nextFence();
		for (auto& [stage, _] : m_batches.active.entries) {
			commands.push_back(stage.command);
		}
		vk::SubmitInfo submitInfo;
		submitInfo.commandBufferCount = (u32)commands.size();
		submitInfo.pCommandBuffers = commands.data();
		m_memory.get().m_device.get().queues().submit(QType::eTransfer, submitInfo, m_batches.active.done, true);
		m_batches.submitted.push_back(std::move(m_batches.active));
	}
	m_batches.active = {};
	return m_batches.submitted.size();
}

Transfer::Stage Transfer::newStage(vk::DeviceSize bufferSize) {
	return Stage{nextBuffer(bufferSize), nextCommand()};
}

void Transfer::addStage(Stage&& stage, Promise&& promise) {
	auto lock = m_sync.mutex.lock();
	m_batches.active.entries.emplace_back(std::move(stage), std::move(promise));
}

Buffer Transfer::nextBuffer(vk::DeviceSize size) {
	auto lock = m_sync.mutex.lock();
	for (auto iter = m_data.buffers.begin(); iter != m_data.buffers.end(); ++iter) {
		if (iter->writeSize() >= size) {
			Buffer ret = std::move(*iter);
			m_data.buffers.erase(iter);
			return ret;
		}
	}
	return createStagingBuffer(m_memory, size);
}

vk::CommandBuffer Transfer::nextCommand() {
	auto lock = m_sync.mutex.lock();
	if (!m_data.commands.empty()) {
		auto ret = m_data.commands.back();
		m_data.commands.pop_back();
		return ret;
	}
	vk::CommandBufferAllocateInfo commandBufferInfo;
	commandBufferInfo.commandBufferCount = 1;
	commandBufferInfo.commandPool = m_data.pool;
	return m_memory.get().m_device.get().device().allocateCommandBuffers(commandBufferInfo).front();
}

void Transfer::scavenge(Stage&& stage, vk::Fence fence) {
	m_data.commands.push_back(std::move(stage.command));
	m_data.buffers.push_back(std::move(stage.buffer));
	if (std::find(m_data.fences.begin(), m_data.fences.end(), fence) == m_data.fences.end()) {
		m_memory.get().m_device.get().resetFence(fence);
		m_data.fences.push_back(fence);
	}
}

vk::Fence Transfer::nextFence() {
	if (!m_data.fences.empty()) {
		auto ret = m_data.fences.back();
		m_data.fences.pop_back();
		return ret;
	}
	return m_memory.get().m_device.get().createFence(false);
}
} // namespace le::graphics