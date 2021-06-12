#pragma once
#include <core/not_null.hpp>
#include <core/std_types.hpp>
#include <graphics/render/swapchain.hpp>
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
	struct Fnc {
		vk::Fence fence;
		std::size_t index{};
	};

	enum class State { eReady, eBusy };

	struct Fence : vk::Fence {

		State previous{};

		State state(vk::Device device) const;
	};

	RenderFence(not_null<Device*> device, Buffering buffering = 2_B);
	RenderFence(RenderFence&&) = default;
	RenderFence& operator=(RenderFence&&);
	~RenderFence();

	Buffering buffering() const noexcept { return {(u8)m_storage.fences.size()}; }

	std::size_t index() const noexcept { return m_storage.index; }

	void wait();
	kt::result<Swapchain::Acquire> acquire(Swapchain& swapchain, RenderSemaphore rs);
	bool present(Swapchain& swapchain, vk::SubmitInfo const& info, RenderSemaphore rs);
	void refresh();

	not_null<Device*> m_device;

  private:
	Fence& current() noexcept { return m_storage.fences[index()]; }
	Fence associate(u32 imageIndex);
	void next() noexcept;

	Fence drawFence();
	Fence submitFence();
	void destroy();

	struct {
		kt::fixed_vector<Fence, 4> fences;
		Fence* ptrs[4] = {};
		std::size_t index = 0;
	} m_storage;
};
} // namespace le::graphics
