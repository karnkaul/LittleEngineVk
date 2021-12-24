#pragma once
#include <graphics/geometry.hpp>
#include <graphics/texture.hpp>

namespace le::graphics {
class TextureAtlas {
  public:
	enum class Result { eOk, eOverflowX, eSizeLocked, eResizeFail };

	struct CreateInfo;
	using ID = u32;

	TextureAtlas(not_null<VRAM*> vram, CreateInfo const& info);

	Result add(ID id, Bitmap const& bitmap, CommandBuffer const& cb);
	bool setUV(ID id, Span<Vertex> quad) const noexcept;
	Texture const& texture() const noexcept { return m_texture; }

	bool contains(ID id) const noexcept { return m_data.entries.contains(id); }
	QuadTex get(ID id) const noexcept;
	std::size_t size() const noexcept { return m_data.entries.size(); }
	bool empty() const noexcept { return m_data.entries.empty(); }
	void clear();

	bool sizeLocked() const noexcept { return m_locked; }
	void lockSize(bool lock) noexcept { m_locked = lock; }

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
	Sampler m_sampler;
	Texture m_texture;
	glm::uvec2 m_pad{};
	bool m_locked = false;
	not_null<VRAM*> m_vram;

	QuadUV getUV(Entry const& entry) const noexcept;
	Result prepAtlas(Extent2D extent, CommandBuffer const& cb);
	void nextRow() noexcept;
};

struct TextureAtlas::CreateInfo {
	glm::uvec2 pad = {1U, 1U};
	u32 maxWidth = 512U;
	u32 initialHeight = 64U;
};
} // namespace le::graphics
