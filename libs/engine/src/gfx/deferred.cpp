#include "deferred.hpp"
#include "renderer_impl.hpp"
#include "utils.hpp"
#include "vram.hpp"
#include "window/window_impl.hpp"

namespace le::gfx
{
namespace
{
struct Deferred
{
	struct Entry
	{
		u64 lastFrame = 0;
		s16 remaining = 0;
	};
	using WinID = s32;

	std::function<void()> func;
	std::unordered_map<WinID, Entry> drawingMap;
};
std::deque<Deferred> g_deferred;

std::mutex g_mutex;

bool isStale(Deferred& out_deferred, std::unordered_set<s32> const& active)
{
	for (auto& [window, entry] : out_deferred.drawingMap)
	{
		bool const bWindowLost = active.find(window) == active.end();
		auto const pRenderer = WindowImpl::rendererImpl(window);
		if (!pRenderer || bWindowLost)
		{
			--entry.remaining;
		}
		bool const bDrawn = entry.remaining <= 0;
		if (!bDrawn && !bWindowLost && pRenderer)
		{
			if (pRenderer->framesDrawn() < entry.lastFrame)
			{
				// Renderer may have reset frame count, no need to track this window any more
				entry.remaining = 0;
			}
			else
			{
				s16 diff = -(s16)(pRenderer->framesDrawn() - entry.lastFrame);
				entry.remaining = diff + (s16)pRenderer->virtualFrameCount();
			}
		}
		if (entry.remaining > 0)
		{
			return false;
		}
	}
	return true;
}
} // namespace

void deferred::release(Buffer buffer)
{
	release([buffer = std::move(buffer)]() { vram::release(std::move(buffer)); });
}

void deferred::release(Image image, vk::ImageView imageView)
{
	release([image = std::move(image), imageView] {
		vram::release(std::move(image));
		vkDestroy(imageView);
	});
}

void deferred::release(vk::Pipeline pipeline, vk::PipelineLayout layout)
{
	release([pipeline, layout]() { vkDestroy(pipeline, layout); });
}

void deferred::release(std::function<void()> func)
{
	Deferred deferred;
	deferred.func = std::move(func);
	std::unique_lock<std::mutex> lock(g_mutex);
	auto const active = WindowImpl::active();
	for (auto const& window : active)
	{
		auto const pRenderer = WindowImpl::rendererImpl(window);
		if (pRenderer)
		{
			deferred.drawingMap[window] = {pRenderer->framesDrawn(), (s16)pRenderer->virtualFrameCount()};
		}
	}
	g_deferred.push_back(std::move(deferred));
}

void deferred::update()
{
	std::unique_lock<std::mutex> lock(g_mutex);
	auto const active = WindowImpl::active();
	auto iter = std::remove_if(g_deferred.begin(), g_deferred.end(), [&](auto& deferred) {
		if (isStale(deferred, active))
		{
			deferred.func();
			return true;
		}
		return false;
	});
	g_deferred.erase(iter, g_deferred.end());
}

void deferred::deinit()
{
	g_info.device.waitIdle();
	for (auto& deferred : g_deferred)
	{
		deferred.func();
	}
	g_deferred = {};
}
} // namespace le::gfx
