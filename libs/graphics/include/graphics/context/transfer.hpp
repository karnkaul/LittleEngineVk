#pragma once
#include <core/not_null.hpp>
#include <core/span.hpp>
#include <core/time.hpp>
#include <graphics/buffer.hpp>
#include <ktl/async_queue.hpp>
#include <ktl/future.hpp>
#include <ktl/kthread.hpp>
#include <ktl/move_only_function.hpp>
#include <list>
#include <memory>
#include <vector>

namespace le::graphics {
constexpr vk::DeviceSize operator""_MB(unsigned long long size) { return size << 20; }

class Transfer final : public Pinned {
  public:
	using notify_t = void;
	using Promise = ktl::promise<notify_t>;
	using Future = ktl::future<notify_t>;

	struct MemRange final {
		vk::DeviceSize size = 2;
		std::size_t count = 1;
	};
	static constexpr std::array<MemRange, 1> defaultReserve = {{{8_MB, 4}}};

	struct CreateInfo;

	Transfer(not_null<Memory*> memory, CreateInfo const& info);
	~Transfer();

	std::size_t update();
	bool polling() const noexcept { return m_sync.poll.active(); }

  private:
	struct Stage final {
		std::optional<Buffer> buffer;
		vk::CommandBuffer command;
	};

	struct Batch final {
		using Entry = std::pair<Stage, Promise>;

		std::vector<Entry> entries;
		vk::Fence done;
		u8 framePad = 0;
	};

	Stage newStage(vk::DeviceSize bufferSize);
	void addStage(Stage&& stage, Promise&& promise);

	void scavenge(Stage&& stage, vk::Fence fence);
	vk::Fence nextFence();
	std::optional<Buffer> nextBuffer(vk::DeviceSize size);
	vk::CommandBuffer nextCommand();
	void stopPolling();
	void stopTransfer();

	struct {
		vk::CommandPool pool;
		std::vector<vk::CommandBuffer> commands;
		std::vector<vk::Fence> fences;
		std::list<Buffer> buffers;
	} m_data;
	struct {
		ktl::kthread staging;
		ktl::kthread poll;
		std::mutex mutex;
	} m_sync;
	struct {
		Batch active;
		std::vector<Batch> submitted;
	} m_batches;
	ktl::async_queue<ktl::move_only_function<void()>> m_queue;
	not_null<Memory*> m_memory;

	friend class VRAM;
};

struct Transfer::CreateInfo {
	Span<MemRange const> reserve = defaultReserve;
	std::optional<Time_ms> autoPollRate = 3ms;
};
} // namespace le::graphics
