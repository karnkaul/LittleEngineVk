#pragma once
#include <engine/resources/resources.hpp>

namespace le
{
class Text2D
{
public:
	struct Info
	{
		stdfs::path id;
		res::Font::Text data;
		res::Font font;
	};

public:
	Text2D() = default;
	Text2D(Text2D&&) = default;
	Text2D& operator=(Text2D&&) = default;
	virtual ~Text2D() = default;

public:
	bool setup(Info info);

	void updateText(res::Font::Text data);
	void updateText(std::string text);

	res::Mesh mesh() const;
	bool ready() const;
	bool busy() const;

protected:
	res::Font::Text m_data;
	res::Font m_font;
	res::Scoped<res::Mesh> m_mesh;
};
} // namespace le
