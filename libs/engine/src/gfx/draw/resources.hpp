#pragma once
#include "gfx/common.hpp"
#include "shader.hpp"
#include "pipeline.hpp"

namespace le::gfx::resources
{
TResult<Shader*> create(stdfs::path const& id, ShaderData const& data);

template <typename T>
TResult<T*> get(stdfs::path const&)
{
	static_assert(FalseType<T>::value, "Invalid type!");
}

template <typename T>
bool unload(stdfs::path const&)
{
	static_assert(FalseType<T>::value, "Invalid type!");
}

template <typename T>
void unloadAll()
{
	static_assert(FalseType<T>::value, "Invalid type!");
}

template <>
TResult<Shader*> get<Shader>(stdfs::path const& id);
template <>
bool unload<Shader>(stdfs::path const& id);
template <>
void unloadAll<Shader>();

void update();
void unloadAll();
} // namespace le::gfx::resources
