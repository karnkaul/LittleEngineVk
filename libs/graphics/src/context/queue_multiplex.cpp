#include <map>
#include <set>
#include <graphics/common.hpp>
#include <graphics/context/device.hpp>
#include <graphics/context/instance.hpp>
#include <graphics/context/queue_multiplex.hpp>

namespace le::graphics {
namespace {
using vkqf = vk::QueueFlagBits;

class Selector {
  public:
	Selector(std::vector<QueueMultiplex::Family> families) : m_families(std::move(families)) {
		QFlags found;
		for (auto const& family : m_families) { found.set(family.flags); }
		bool const valid = found.all(qflags_all);
		ensure(valid, "Required queues not present");
		if (!valid) { g_log.log(lvl::error, 0, "[{}] Required Vulkan Queues not present on selected physical device!"); }
	}

	QueueMultiplex::Family* exact(QFlags flags) {
		for (auto& f : m_families) {
			// Only return if queue flags match exactly
			if (f.flags == flags && f.reserved < f.total) { return &f; }
		}
		return nullptr;
	}

	QueueMultiplex::Family* best(QFlags flags) {
		for (auto& f : m_families) {
			// Return if queue supports desired flags
			if (f.flags.all(flags) && f.reserved < f.total) { return &f; }
		}
		return nullptr;
	}

	template <typename T>
	using stdil = std::initializer_list<T>;

	QueueMultiplex::Family* reserve(stdil<QFlags> combo) {
		QueueMultiplex::Family* f = nullptr;
		// First pass: exact match
		for (QFlags flags : combo) {
			if (f = exact(flags); f && f->reserved < f->total) { break; }
		}
		if (!f || f->reserved >= f->total) {
			// Second pass: best match
			for (QFlags flags : combo) {
				if (f = best(flags); f && f->reserved < f->total) { break; }
			}
		}
		if (f && f->reserved < f->total) {
			// Match found, reserve queue and return
			++f->reserved;
			return f;
		}
		// No matches found / no queues left to reserve
		return nullptr;
	}

	std::vector<QueueMultiplex::Family> m_families;
	u32 m_queueCount = 0;
};

QueueMultiplex::QCI createInfo(QueueMultiplex::Family& out_family, Span<f32 const> prio) {
	QueueMultiplex::QCI ret;
	ret.first.queueFamilyIndex = out_family.familyIndex;
	ret.first.queueCount = (u32)prio.size();
	ret.first.pQueuePriorities = prio.data();
	for (std::size_t i = 0; i < prio.size(); ++i) {
		QueueMultiplex::Queue queue;
		ensure(out_family.nextQueueIndex < out_family.total, "No queues remaining");
		queue.arrayIndex = out_family.nextQueueIndex++;
		queue.familyIndex = out_family.familyIndex;
		queue.flags = out_family.flags;
		ret.second.push_back(queue);
	}
	return ret;
}

template <typename T, typename... Ts>
constexpr bool uniqueFam(T&& t, Ts&&... ts) noexcept {
	return ((t.familyIndex != ts.familyIndex) && ...);
}
template <typename T, typename... Ts>
constexpr bool uniqueQueue(T&& t, Ts&&... ts) noexcept {
	return ((t.familyIndex != ts.familyIndex && t.arrayIndex != ts.arrayIndex) && ...);
}
} // namespace

std::vector<vk::DeviceQueueCreateInfo> QueueMultiplex::select(std::vector<Family> families) {
	std::vector<vk::DeviceQueueCreateInfo> ret;
	Selector sl(std::move(families));
	// Reserve one queue for graphics/present
	auto fpg = sl.reserve({QFlags(QType::eGraphics) | QType::ePresent});
	// Reserve another for transfer
	auto ft = sl.reserve({QType::eTransfer, QFlags(QType::eTransfer) | QType::ePresent, QFlags(QType::eTransfer) | QType::eGraphics});
	// Can't function without graphics/present
	if (!fpg) { return ret; }
	if (ft && uniqueFam(*fpg, *ft)) {
		// Two families, two queues
		static std::array const prio = {1.0f};
		makeQueues(ret, makeFrom2(*fpg, *ft, prio, prio), {{{0, 0}, {0, 0}, {1, 0}}});
	} else {
		if (fpg->total > 1) {
			// One family, two queues
			static std::array const prio = {0.8f, 0.2f};
			makeQueues(ret, makeFrom1(*fpg, prio), {{{0, 0}, {0, 0}, {0, 1}}});
		} else {
			// One family, one queue
			static std::array const prio = {1.0f};
			makeQueues(ret, makeFrom1(*fpg, prio), {{{0, 0}, {0, 0}, {0, 0}}});
		}
	}
	m_queues[QType::eGraphics].second = &m_mutexes.gp;
	m_queues[QType::ePresent].second = &m_mutexes.gp;
	m_queues[QType::eTransfer].second = &m_mutexes.t;
	return ret;
}

void QueueMultiplex::setup(vk::Device device) {
	std::set<u32> families, queues;
	for (auto& [queue, _] : m_queues.arr) {
		queue.queue = device.getQueue(queue.familyIndex, queue.arrayIndex);
		families.insert(queue.familyIndex);
		queues.insert((queue.familyIndex << 4) ^ queue.arrayIndex);
	}
	m_familyCount = (u32)families.size();
	m_queueCount = (u32)queues.size();
	g_log.log(lvl::info, 1, "[{}] Multiplexing [{}] Vulkan queue(s) from [{}] queue families for [Graphics/Present, Transfer]", g_name, m_queueCount,
			  m_familyCount);
}

kt::fixed_vector<u32, 3> QueueMultiplex::familyIndices(QFlags flags) const {
	kt::fixed_vector<u32, 3> ret;
	if (flags.test(QType::eGraphics)) { ret.push_back(queue(QType::eGraphics).familyIndex); }
	if (flags.test(QType::ePresent) && queue(QType::ePresent).familyIndex != queue(QType::eGraphics).familyIndex) {
		ret.push_back(queue(QType::ePresent).familyIndex);
	}
	if (flags.test(QType::eTransfer) && queue(QType::eTransfer).familyIndex != queue(QType::eGraphics).familyIndex) {
		ret.push_back(queue(QType::eTransfer).familyIndex);
	}
	return ret;
}

vk::Result QueueMultiplex::present(vk::PresentInfoKHR const& info, bool bLock) {
	auto& q = queue(QType::ePresent);
	if (!bLock) {
		return q.queue.presentKHR(&info);
	} else {
		std::scoped_lock lock(mutex(QType::ePresent));
		return q.queue.presentKHR(&info);
	}
}

void QueueMultiplex::submit(QType type, vAP<vk::SubmitInfo> infos, vk::Fence signal, bool bLock) {
	auto& q = queue(type);
	if (!bLock) {
		q.queue.submit(infos, signal);
	} else {
		std::scoped_lock lock(mutex(QType::ePresent));
		q.queue.submit(infos, signal);
	}
}

QueueMultiplex::QCIArr<1> QueueMultiplex::makeFrom1(Family& gpt, Span<f32 const> prio) { return {createInfo(gpt, prio)}; }

QueueMultiplex::QCIArr<2> QueueMultiplex::makeFrom2(Family& a, Family& b, Span<f32 const> pa, Span<f32 const> pb) {
	std::array<QueueMultiplex::QCI, 2> ret;
	ret[0] = createInfo(a, pa);
	ret[1] = createInfo(b, pb);
	return ret;
}

QueueMultiplex::QCIArr<3> QueueMultiplex::makeFrom3(Family& g, Family& p, Family& t, Span<f32 const> pg, Span<f32 const> pp, Span<f32 const> pt) {
	std::array<QueueMultiplex::QCI, 3> ret;
	ret[0] = createInfo(g, pg);
	ret[1] = createInfo(p, pp);
	ret[2] = createInfo(t, pt);
	return ret;
}

void QueueMultiplex::makeQueues(qcivec& out_vec, Span<QCI const> qcis, Assign const& a) {
	for (auto const& [info, _] : qcis) { out_vec.push_back(info); }
	assign(qcis[a[0].first].second[a[0].second], qcis[a[1].first].second[a[1].second], qcis[a[2].first].second[a[2].second]);
}

void QueueMultiplex::assign(Queue g, Queue p, Queue t) {
	if (uniqueQueue(g, p, t)) {
		g.bUnique = p.bUnique = t.bUnique = true;
	} else if (uniqueQueue(g, p)) {
		g.bUnique = p.bUnique = true;
	} else if (uniqueQueue(g, t)) {
		g.bUnique = t.bUnique = true;
	} else if (uniqueQueue(p, t)) {
		p.bUnique = t.bUnique = true;
	}
	queue(QType::eGraphics) = g;
	queue(QType::ePresent) = p;
	queue(QType::eTransfer) = t;
}
} // namespace le::graphics
