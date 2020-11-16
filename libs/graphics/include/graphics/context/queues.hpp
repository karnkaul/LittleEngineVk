#pragma once
#include <graphics/types.hpp>
#include <kt/async_queue/async_queue.hpp>

namespace le::graphics {
class Queues final {
  public:
	struct Indices {
		u32 familyIndex = 0;
		u32 arrayIndex = 0;
	};
	struct Queue : Indices {
		vk::Queue queue;
	};

  public:
	void setup(vk::Device device, EnumArray<QType, Indices> data);

	u32 familyIndex(QType type) const noexcept {
		return queue(type).familyIndex;
	}
	u32 arrayIndex(QType type) const noexcept {
		return queue(type).arrayIndex;
	}

	std::vector<u32> familyIndices(QFlags flags) const;
	void submit(QType type, vk::ArrayProxy<vk::SubmitInfo const> const& infos, vk::Fence signal);
	vk::Result present(vk::PresentInfoKHR const& info);
	void waitIdle(QType type) const;

  private:
	Queue& queue(QType type) noexcept {
		return m_queues[(std::size_t)type];
	}

	Queue const& queue(QType type) const noexcept {
		return m_queues[(std::size_t)type];
	}

	EnumArray<QType, Queue> m_queues;
	kt::lockable<> m_mutex;
};
} // namespace le::graphics
