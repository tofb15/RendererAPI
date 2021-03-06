#pragma once
#include "Utills/Math.hpp"
namespace FusionReactor {

	class WindowInput {
	public:
		static const unsigned NUM_KEYS = 256;
		static const unsigned NUM_MOUSE_KEYS = 3;

		enum KEY_CODES {
			MOUSE_KEY_CODE_LEFT = 0,
			MOUSE_KEY_CODE_MIDDLE,
			MOUSE_KEY_CODE_RIGHT,

			//Special
			KEY_CODE_backspace = 8,
			KEY_CODE_TAB = 9,
			KEY_CODE_enter = 13,
			KEY_CODE_SHIFT = 16,
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
			KEY_CODE_1,
			KEY_CODE_2,
			KEY_CODE_3,
			KEY_CODE_4,
			KEY_CODE_5,
			KEY_CODE_6,
			KEY_CODE_7,
			KEY_CODE_8,
			KEY_CODE_9,
			//Letters
			KEY_CODE_A = 65,
			KEY_CODE_B,
			KEY_CODE_C,
			KEY_CODE_D,
			KEY_CODE_E,
			KEY_CODE_F,
			KEY_CODE_G,
			KEY_CODE_H,
			KEY_CODE_I,
			KEY_CODE_J,
			KEY_CODE_K,
			KEY_CODE_L,
			KEY_CODE_M,
			KEY_CODE_N,
			KEY_CODE_O,
			KEY_CODE_P,
			KEY_CODE_Q,
			KEY_CODE_R,
			KEY_CODE_S,
			KEY_CODE_T,
			KEY_CODE_U,
			KEY_CODE_V,
			KEY_CODE_W,
			KEY_CODE_X,
			KEY_CODE_Y,
			KEY_CODE_Z,
			//Function keys
			KEY_CODE_F1 = 112,
			KEY_CODE_F2,
			KEY_CODE_F3,
			KEY_CODE_F4,
			KEY_CODE_F5,
			KEY_CODE_F6,
			KEY_CODE_F7,
			KEY_CODE_F8,
			KEY_CODE_F9,
			KEY_CODE_F10,
			KEY_CODE_F11,
			KEY_CODE_F12,
		};

		//enum MOUSE_KEY_CODES
		//{
		//	MOUSE_KEY_CODE_LEFT = KEY_CODES::MOUSE_KEY_CODE_LEFT,
		//	MOUSE_KEY_CODE_MIDDLE,
		//	MOUSE_KEY_CODE_RIGHT,
		//};

		WindowInput();
		virtual ~WindowInput();

		void Reset();
		void SetKeyDown(char key, bool isDown);
		void SetMouseKeyDown(char key, bool isDown);

		void SetKeyPressed(char key, bool isPressed);
		void SetMouseMovement(const Int2& mouseMovement);
		void SetMouseWheelMovement(int mouseWheelMovement);

		bool IsKeyDown(char key)	const;
		bool IsKeyUp(char key)		const;
		bool IsKeyPressed(char key) const;
		Int2 GetMouseMovement()		const;
		int GetMouseWheelMovement()		const;

	private:
		bool m_isKeysDown[NUM_KEYS];
		bool m_isKeysPressed[NUM_KEYS];

		//bool m_isMouseKeysDown[NUM_MOUSE_KEYS];
		//bool m_isMouseKeysPressed[NUM_MOUSE_KEYS];

		Int2 m_mouseMovement;
		int m_mouseWheelMovement;
	};
}