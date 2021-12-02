#pragma once
#include <core/hash.hpp>
#include <core/not_null.hpp>
#include <graphics/render/rgba.hpp>

namespace le {
namespace graphics {
class DescriptorUpdater;
class Texture;
} // namespace graphics
class AssetStore;

using RGBA = graphics::RGBA;

struct Material2 {
	Hash map_Kd;
	Hash map_Ks;
	Hash map_d;
	Hash map_Bump;
	RGBA Ka = colours::white;
	RGBA Kd = colours::white;
	RGBA Ks = colours::black;
	RGBA Tf = colours::white;
	f32 Ns = 42.0f;
	f32 d = 1.0f;
	int illum = 2;
};

class IMaterial {
  public:
	using Texture = graphics::Texture;
	using DescriptorUpdater = graphics::DescriptorUpdater;

	class Helper;

	virtual ~IMaterial() = default;

	virtual void update(Helper const& helper, Material2 const& mat) const = 0;
};

class IMaterial::Helper {
  public:
	Helper(not_null<AssetStore const*> store, not_null<DescriptorUpdater*> descriptor) noexcept : m_store(store), m_descriptor(descriptor) {}

	AssetStore const& store() const noexcept { return *m_store; }
	Texture const* texture(Hash uri) const;
	DescriptorUpdater& descriptor() const noexcept { return *m_descriptor; }

  private:
	mutable std::unordered_map<Hash, Texture const*> m_textures;
	not_null<AssetStore const*> m_store;
	not_null<DescriptorUpdater*> m_descriptor;
};
} // namespace le
