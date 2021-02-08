#pragma once
#include <future>
#include <list>
#include <memory>
#include <vector>
#include <core/ref.hpp>
#include <core/span.hpp>
#include <core/threads.hpp>
#include <core/time.hpp>
#include <graphics/resources.hpp>
#include <kt/async_queue/async_queue.hpp>

namespace le::graphics {
constexpr vk::DeviceSize operator""_MB(unsigned long long size) {
	return size << 20;
}

class Transfer final {
  public:
	using notify_t = void;
	using Promise = std::shared_ptr<std::promise<notify_t>>;
	using Future = std::future<notify_t>;

	struct MemRange final {
		vk::DeviceSize size = 2;
		std::size_t count = 1;
	};
	static constexpr std::array<MemRange, 1> defaultReserve = {{{8_MB, 4}}};

	struct CreateInfo;

	struct Stage final {
		Buffer buffer;
		vk::CommandBuffer command;
	};

	struct Batch final {
		using Entry = std::pair<Stage, Promise>;

		std::vector<Entry> entries;
		vk::Fence done;
		u8 framePad = 0;
	};

	Transfer(Memory& memory, CreateInfo const& info);
	~Transfer();

	static Promise makePromise() noexcept;

	std::size_t update();

	Stage newStage(vk::DeviceSize bufferSize);
	void addStage(Stage&& stage, Promise&& promise);

	bool polling() const noexcept {
		return m_sync.bPoll.load();
	}

  private:
	void scavenge(Stage&& stage, vk::Fence fence);
	vk::Fence nextFence();
	Buffer nextBuffer(vk::DeviceSize size);
	vk::CommandBuffer nextCommand();

	struct {
		vk::CommandPool pool;
		std::vector<vk::CommandBuffer> commands;
		std::vector<vk::Fence> fences;
		std::list<Buffer> buffers;
	} m_data;
	struct {
		std::optional<threads::TScoped> stagingThread;
		std::optional<threads::TScoped> pollThread;
		kt::lockable_t<> mutex;
		std::atomic<bool> bPoll;
	} m_sync;
	struct {
		Batch active;
		std::vector<Batch> submitted;
	} m_batches;
	kt::async_queue<std::function<void()>> m_queue;
	Ref<Memory> m_memory;

	friend class VRAM;
};

struct Transfer::CreateInfo {
	View<MemRange> reserve = defaultReserve;
	std::optional<Time_ms> autoPollRate = 3ms;
};

// impl
inline Transfer::Promise Transfer::makePromise() noexcept {
	return std::make_shared<Promise::element_type>();
}
} // namespace le::graphics
