#pragma once
#include <engine/resources/resource_types.hpp>

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

protected:
	res::Font::Text m_data;
	res::Mesh m_mesh;
	res::Font m_font;

public:
	Text2D();
	Text2D(Text2D&&);
	Text2D& operator=(Text2D&&);
	virtual ~Text2D();

public:
	bool setup(Info info);

	void updateText(res::Font::Text data);
	void updateText(std::string text);

	res::Mesh mesh() const;
	bool isReady() const;
	bool isBusy() const;
};
} // namespace le
