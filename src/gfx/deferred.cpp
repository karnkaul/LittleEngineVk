#include <gfx/deferred.hpp>
#include <gfx/device.hpp>
#include <gfx/render_driver_impl.hpp>
#include <gfx/vram.hpp>
#include <kt/async_queue/async_queue.hpp>
#include <window/window_impl.hpp>

namespace le::gfx {
namespace {
struct Deferred {
	struct Entry {
		u64 lastFrame = 0;
		s16 remaining = 0;
		u8 pad = 0;
	};
	using WinID = s32;

	std::function<void()> func;
	std::unordered_map<WinID, Entry> drawingMap;
};

std::deque<Deferred> g_deferred;
kt::lockable<std::mutex> g_mutex;

bool isStale(Deferred& out_deferred, std::unordered_set<s32> const& active) {
	for (auto& [window, entry] : out_deferred.drawingMap) {
		bool const bWindowLost = active.find(window) == active.end();
		auto const pDriver = WindowImpl::driverImpl(window);
		if (!pDriver || bWindowLost) {
			--entry.remaining;
		}
		bool const bDrawn = entry.remaining <= 0;
		if (!bDrawn && !bWindowLost && pDriver) {
			if (pDriver->framesDrawn() < entry.lastFrame) {
				// Renderer may have reset frame count, no need to track this window any more
				entry.remaining = 0;
			} else {
				s16 diff = (s16)(pDriver->framesDrawn() - entry.lastFrame);
				entry.remaining = (s16)pDriver->virtualFrameCount() + (s16)entry.pad - diff;
			}
		}
		if (entry.remaining > 0) {
			return false;
		}
	}
	return true;
}
} // namespace

void deferred::release(Buffer buffer) {
	release([buffer = std::move(buffer)]() { vram::release(std::move(buffer)); });
}

void deferred::release(Image image, vk::ImageView imageView) {
	release([image = std::move(image), imageView] {
		vram::release(std::move(image));
		g_device.destroy(imageView);
	});
}

void deferred::release(vk::Pipeline pipeline, vk::PipelineLayout layout) {
	release([pipeline, layout]() { g_device.destroy(pipeline, layout); });
}

void deferred::release(std::function<void()> func, u8 extraFrames) {
	Deferred deferred;
	deferred.func = std::move(func);
	auto lock = g_mutex.lock();
	auto const active = WindowImpl::allExisting();
	for (auto const& window : active) {
		auto const pDriver = WindowImpl::driverImpl(window);
		if (pDriver) {
			deferred.drawingMap[window] = {pDriver->framesDrawn(), (s16)pDriver->virtualFrameCount(), extraFrames};
		}
	}
	g_deferred.push_back(std::move(deferred));
}

void deferred::update() {
	auto lock = g_mutex.lock();
	auto const active = WindowImpl::allExisting();
	auto iter = std::remove_if(g_deferred.begin(), g_deferred.end(), [&](auto& deferred) {
		if (isStale(deferred, active)) {
			deferred.func();
			return true;
		}
		return false;
	});
	g_deferred.erase(iter, g_deferred.end());
}

void deferred::deinit() {
	flush();
}

void deferred::flush() {
	for (auto& deferred : g_deferred) {
		deferred.func();
	}
	g_deferred = {};
}
} // namespace le::gfx
