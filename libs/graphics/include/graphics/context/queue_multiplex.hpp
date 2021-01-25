#pragma once
#include <core/span.hpp>
#include <core/std_types.hpp>
#include <graphics/qflags.hpp>
#include <kt/async_queue/lockable.hpp>
#include <vulkan/vulkan.hpp>

namespace le::graphics {
class Instance;

class QueueMultiplex final {
  public:
	struct Family {
		u32 familyIndex = 0;
		u32 total = 0;
		u32 reserved = 0;
		u32 nextQueueIndex = 0;
		QFlags flags;
	};
	struct Indices {
		u32 familyIndex = 0;
		u32 arrayIndex = 0;
	};
	struct Queue : Indices {
		vk::Queue queue;
		QFlags flags;
		bool bUnique = false;
	};

	using QCI = std::pair<vk::DeviceQueueCreateInfo, std::vector<QueueMultiplex::Queue>>;

  public:
	std::vector<vk::DeviceQueueCreateInfo> select(std::vector<Family> families);
	void setup(vk::Device device);

	u32 familyIndex(QType type) const noexcept {
		return queue(type).familyIndex;
	}
	u32 arrayIndex(QType type) const noexcept {
		return queue(type).arrayIndex;
	}

	std::vector<u32> familyIndices(QFlags flags) const;

	void submit(QType type, vk::ArrayProxy<vk::SubmitInfo const> const& infos, vk::Fence signal, bool bLock);
	vk::Result present(vk::PresentInfoKHR const& info, bool bLock);

	u32 familyCount() const noexcept {
		return m_familyCount;
	}
	u32 queueCount() const noexcept {
		return m_queueCount;
	}

	template <template <typename...> typename L = std::scoped_lock>
	auto lockMutex(QType type) {
		return mutex(type).lock<L>();
	}

	using Lock = std::scoped_lock<std::mutex, std::mutex>;
	Lock lock() {
		return Lock(m_mutexes.gp.mutex, m_mutexes.t.mutex);
	}

	Queue& queue(QType type) noexcept {
		return m_queues[(std::size_t)type].first;
	}

	Queue const& queue(QType type) const noexcept {
		return m_queues[(std::size_t)type].first;
	}

  private:
	kt::lockable<>& mutex(QType type) {
		return *m_queues[(std::size_t)type].second;
	}

	template <std::size_t N>
	using QCIArr = std::array<QCI, N>;
	QCIArr<1> makeFrom1(Family& gpt, Span<f32> prio);
	QCIArr<2> makeFrom2(Family& a, Family& b, Span<f32> pa, Span<f32> pb);
	QCIArr<3> makeFrom3(Family& g, Family& p, Family& t, Span<f32> pg, Span<f32> pp, Span<f32> pt);

	using qcivec = std::vector<vk::DeviceQueueCreateInfo>;
	using Assign = std::array<std::pair<std::size_t, std::size_t>, 3>;
	void makeQueues(qcivec& out_vec, Span<QCI> qcis, Assign const& a);

	void assign(Queue g, Queue p, Queue t);

	EnumArray<QType, std::pair<Queue, kt::lockable<>*>> m_queues;
	struct {
		kt::lockable<> gp;
		kt::lockable<> t;
	} m_mutexes;
	u32 m_familyCount = 0;
	u32 m_queueCount = 0;
};

} // namespace le::graphics
