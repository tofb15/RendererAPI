#include "Window.hpp"

bool Window::ContainsMouse()
{
	return false;
}

bool Window::IsInFocus()
{
	return false;
}

WindowInput & Window::GetWindowInputHandler()
{
	return m_input;
}

Window::Window()
{
}

Window::~Window()
{
}

const char * Window::GetTitle() const
{
	return m_title;
}
