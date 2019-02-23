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
	return GLOBAL_WINODW_INPUT_HANDLER;
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
