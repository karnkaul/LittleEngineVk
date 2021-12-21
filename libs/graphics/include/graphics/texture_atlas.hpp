#pragma once
#include <graphics/geometry.hpp>
#include <graphics/texture.hpp>

namespace le::graphics {
class TextureAtlas {
  public:
	struct CreateInfo;
	struct Img {
		UVQuad uv{};
		Extent2D extent{};
	};
	using ID = u32;

	TextureAtlas(not_null<VRAM*> vram, CreateInfo const& info);

	bool add(ID id, Bitmap const& bitmap, CommandBuffer const& cb);
	bool setUV(ID id, Span<Vertex> quad) const noexcept;
	Texture const& texture() const noexcept { return m_texture; }

	bool contains(ID id) const noexcept { return m_data.entries.contains(id); }
	Img get(ID id) const noexcept;
	std::size_t size() const noexcept { return m_data.entries.size(); }
	bool empty() const noexcept { return m_data.entries.empty(); }

  private:
	struct Entry {
		Extent2D extent{};
		Extent2D offset{};
	};
	struct {
		std::unordered_map<ID, Entry> entries;
		Extent2D head{};
		u32 rowHeight = 0;
	} m_data;
	Texture m_texture;
	not_null<VRAM*> m_vram;

	UVQuad getUV(Entry const& entry) const noexcept;
	bool prepAtlas(Extent2D extent, CommandBuffer const& cb);
	void nextRow() noexcept;
};

struct TextureAtlas::CreateInfo {
	vk::Sampler sampler;
	u32 maxWidth = 1024U;
	u32 initialHeight = 256U;
};
} // namespace le::graphics
