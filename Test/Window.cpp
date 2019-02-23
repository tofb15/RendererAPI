#include "Window.hpp"

bool Window::ContainsMouse()
{
	return false;
}

bool Window::IsInFocus()
{
	return false;
}

WindowInput & Window::GetLocalWindowInputHandler()
{
	return m_input;
}

WindowInput & Window::GetGlobalWindowInputHandler()
{
	return __GLOBAL_WINODW_INPUT_HANDLER;
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

Int2 Window::GetDimensions() const
{
	return m_dimensions;
}

Int2 Window::GetPosition() const
{
	return m_position;
}
