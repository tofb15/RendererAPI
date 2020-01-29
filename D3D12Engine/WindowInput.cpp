#include "stdafx.h"

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
		//m_isKeysDown[i] = false;
		m_isKeysPressed[i] = false;
	}
}

void WindowInput::SetKeyDown(char key, bool isDown)
{
#ifdef _DEBUG
	if (key <= MOUSE_KEY_CODE_RIGHT) {
		m_isKeysDown[-1] = false; //keycode 0,1,2 is reserved for mouse. Call SetMouseKeyDown insteed
	}
#endif // DEBUG

	if(key < NUM_KEYS)
		m_isKeysDown[key] = isDown;
}

void WindowInput::SetMouseKeyDown(char key, bool isDown)
{
#ifdef _DEBUG
	if (key > MOUSE_KEY_CODE_RIGHT) {
		m_isKeysDown[-1] = false; //keycode 0,1,2 is reserved for mouse. Call SetKeyDown insteed
	}
#endif // DEBUG

	if (m_isKeysDown[key] && !isDown)
		m_isKeysPressed[key] = true;
	else
		m_isKeysPressed[key] = false;

	m_isKeysDown[key] = isDown;
}

void WindowInput::SetKeyPressed(char key, bool isPressed)
{
#ifdef _DEBUG
	if (key <= MOUSE_KEY_CODE_RIGHT) {
		m_isKeysDown[-1] = false; //keycode 0,1,2 is reserved for mouse. Call SetMouseKeyDown insteed
	}
#endif // DEBUG

	if (key < NUM_KEYS)
		m_isKeysPressed[key] = isPressed;
}

void WindowInput::SetMouseMovement(Int2 mouseMovement)
{
	m_mouseMovement = mouseMovement;
}

void WindowInput::SetMouseWheelMovement(int mouseWheelMovement)
{
	m_mouseWheelMovement = mouseWheelMovement;
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

Int2 WindowInput::GetMouseMovement() const
{
	return m_mouseMovement;
}

int WindowInput::GetMouseWheelMovement() const
{
	return m_mouseWheelMovement;
}
