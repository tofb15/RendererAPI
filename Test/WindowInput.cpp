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
	if(key < NUM_KEYS)
		m_isKeysDown[key] = isDown;
}

void WindowInput::SetKeyPressed(char key, bool isPressed)
{
	if (key < NUM_KEYS)
		m_isKeysPressed[key] = isPressed;
}

bool WindowInput::IsKeyDown(char key) const
{
	if (key < NUM_KEYS)
		return m_isKeysDown[key];
	else
		return false;
}

bool WindowInput::IsKeyUp(char key) const
{
	if (key < NUM_KEYS)
		return !m_isKeysDown[key];
	else
		return false;
}

bool WindowInput::IsKeyPressed(char key) const
{
	if (key < NUM_KEYS)
		return m_isKeysPressed[key];
	else
		return false;
}
