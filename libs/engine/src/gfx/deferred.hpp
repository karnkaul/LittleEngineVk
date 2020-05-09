#pragma once
#include <functional>
#include "common.hpp"

namespace le::gfx::deferred
{
void release(Buffer buffer);
void release(Image image, vk::ImageView imageView = {});
void release(vk::Pipeline pipeline, vk::PipelineLayout layout);

void release(std::function<void()> func);

void update();
void deinit();
} // namespace le::gfx::deferred
