#pragma once
#include <memory>
#include <string>
#include <glm/glm.hpp>
#include "core/zero.hpp"

namespace  le
{
class Window final
{
public:
	using ID = TZero<s32, -1>;

public:
	enum class Mode
	{
		DecoratedWindow = 0,
		BorderlessWindow,
		BorderlessFullscreen,
		DedicatedFullscreen,
		COUNT_
	};

	struct Data
	{
		std::string title;
		glm::ivec2 size = {};
		s32 screenID = 0;
		Mode mode = Mode::DecoratedWindow;
	};

private:
	std::unique_ptr<class WindowImpl> m_uImpl;
	ID m_id = ID::Null;

public:
	Window();
	Window(Window&&);
	Window& operator=(Window&&);
	~Window();

public:
	ID id() const;
	bool isOpen() const;
	bool isClosing() const;

	bool create(Data const& data);
	void pollEvents();
	void close();
	void destroy();
};
} // namespace  le
