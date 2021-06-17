#pragma once
#include <core/not_null.hpp>
#include <core/std_types.hpp>
#include <graphics/render/swapchain.hpp>
#include <graphics/utils/deferred.hpp>
#include <kt/fixed_vector/fixed_vector.hpp>
#include <vulkan/vulkan.hpp>

namespace le::graphics {
class Device;

struct RenderSemaphore {
	vk::Semaphore draw;
	vk::Semaphore present;
};

class RenderFence {
  public:
	enum class State { eReady, eBusy };

	struct Fence {
		Deferred<vk::Fence> fence;
		State previous{};

		State state(vk::Device device) const;
	};

	RenderFence(not_null<Device*> device, Buffering buffering = 2_B);

	Buffering buffering() const noexcept { return {(u8)m_storage.fences.size()}; }
	std::size_t index() const noexcept { return m_storage.index; }

	void wait();
	kt::result<Swapchain::Acquire> acquire(Swapchain& swapchain, vk::Semaphore wait);
	bool present(Swapchain& swapchain, vk::SubmitInfo const& info, vk::Semaphore wait);
	void refresh();

	not_null<Device*> m_device;

  private:
	Fence& current() noexcept { return m_storage.fences[index()]; }
	void associate(u32 imageIndex);
	void next() noexcept;

	vk::Fence drawFence();
	vk::Fence submitFence();

	struct {
		kt::fixed_vector<Fence, 4> fences;
		Fence* ptrs[4] = {};
		std::size_t index = 0;
	} m_storage;
};
} // namespace le::graphics
