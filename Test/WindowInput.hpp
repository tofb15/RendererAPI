#pragma once

class WindowInput
{
public:
	WindowInput();
	~WindowInput();

	void Reset();
	void SetKeyDown(char key, bool isDown);
	void SetKeyPressed(char key, bool isPressed);

	bool IsKeyDown(char key)	const;
	bool IsKeyUp(char key)		const;
	bool IsKeyPressed(char key) const;

private:
	static const unsigned NUM_KEYS = 256;

	bool m_isKeysDown[NUM_KEYS];
	bool m_isKeysPressed[NUM_KEYS];

};
