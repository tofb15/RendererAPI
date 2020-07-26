#include "stdafx.h"

#include "Window.hpp"

bool Window::ContainsMouse() {
	return false;
}

bool Window::IsInFocus() {
	return false;
}

WindowInput& Window::GetLocalWindowInputHandler() {
	return m_input;
}

WindowInput& Window::GetGlobalWindowInputHandler() {
	return __GLOBAL_WINODW_INPUT_HANDLER;
}

Float2 Window::PixelCoordToNormalizedDeviceCoord(const Int2& pixelCoord) {
	return Float2((2.0 * (float)pixelCoord.x / m_dimensions.x) - 1.0, (2.0 * (float)pixelCoord.y / m_dimensions.y) - 1.0);
}

Window::Window() {

}

Window::~Window() {

}

const char* Window::GetTitle() const {
	return m_title;
}

Int2 Window::GetDimensions() const {
	return m_dimensions;
}

Int2 Window::GetPosition() const {
	return m_position;
}
