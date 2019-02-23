#include "WindowInput.hpp"

WindowInput::WindowInput()
{
	Reset();
}

WindowInput::~WindowInput()
{
}

void WindowInput::Reset()
{
	for (size_t i = 0; i < NUM_KEYS; i++)
	{
		m_isKeysDown[i] = false;
		m_isKeysPressed[i] = false;
	}
}

void WindowInput::SetKeyDown(char key, bool isDown)
{
	m_isKeysDown[key] = isDown;
}

void WindowInput::SetKeyPressed(char key, bool isPressed)
{
	m_isKeysPressed[key] = isPressed;
}

bool WindowInput::IsKeyDown(char key) const
{
	return m_isKeysDown[key];
}

bool WindowInput::IsKeyUp(char key) const
{
	return !m_isKeysDown[key];
}

bool WindowInput::IsKeyPressed(char key) const
{
	return m_isKeysPressed[key];
}
