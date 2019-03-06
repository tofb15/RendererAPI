#pragma once
#include "Math.hpp"

class WindowInput
{
public:
	static const unsigned NUM_KEYS = 256;

	enum KEY_CODES
	{
		//Special
		KEY_CODE_backspace = 8,
		KEY_CODE_TAB = 9,
		KEY_CODE_enter = 13,
		KEY_CODE_SKIFT = 16,
		KEY_CODE_CTRL = 17,
		KEY_CODE_CAPS = 20,
		Key_CODE_ESC = 27,
		KEY_CODE_SPACE = 32,
		//Arrows
		KEY_CODE_LEFT = 37,
		KEY_CODE_UP = 38,
		KEY_CODE_RIGHT = 39,
		KEY_CODE_DOWN = 40,
		//Numbers
		KEY_CODE_0 = 48,
		KEY_CODE_1 = 49,
		KEY_CODE_2 = 50,
		KEY_CODE_3 = 51,
		KEY_CODE_4 = 52,
		KEY_CODE_5 = 53,
		KEY_CODE_6 = 54,
		KEY_CODE_7 = 55,
		KEY_CODE_8 = 56,
		KEY_CODE_9 = 57,
		//Letters
		KEY_CODE_A = 65,
		KEY_CODE_B = 66,
		KEY_CODE_C = 67,
		KEY_CODE_D = 68,
		KEY_CODE_E = 69,
		KEY_CODE_F = 70,
		KEY_CODE_G = 71,
		KEY_CODE_H = 72,
		KEY_CODE_I = 73,
		KEY_CODE_J = 74,
		KEY_CODE_K = 75,
		KEY_CODE_L = 76,
		KEY_CODE_M = 77,
		KEY_CODE_N = 78,
		KEY_CODE_O = 79,
		KEY_CODE_P = 80,
		KEY_CODE_Q = 81,
		KEY_CODE_R = 82,
		KEY_CODE_S = 83,
		KEY_CODE_T = 84,
		KEY_CODE_U = 85,
		KEY_CODE_V = 86,
		KEY_CODE_W = 87,
		KEY_CODE_X = 88,
		KEY_CODE_Y = 89,
		KEY_CODE_Z = 90,
		//Function keys
		KEY_CODE_F1 = 112,
		KEY_CODE_F2 = 113,
		KEY_CODE_F3 = 114,
		KEY_CODE_F4 = 115,
		KEY_CODE_F5 = 116,
		KEY_CODE_F6 = 117,
		KEY_CODE_F7 = 118,
		KEY_CODE_F8 = 119,
		KEY_CODE_F9 = 120,
		KEY_CODE_F10 = 121,
		KEY_CODE_F11 = 122,
		KEY_CODE_F12 = 123,
	};

	WindowInput();
	~WindowInput();

	void Reset();
	void SetKeyDown(char key, bool isDown);
	void SetKeyPressed(char key, bool isPressed);
	void SetMouseMovement(Int2 mouseMovement);

	bool IsKeyDown(char key)	const;
	bool IsKeyUp(char key)		const;
	bool IsKeyPressed(char key) const;
	Int2 GetMouseMovement()		const;
private:
	bool m_isKeysDown[NUM_KEYS];
	bool m_isKeysPressed[NUM_KEYS];
	Int2 m_mouseMovement;
};
