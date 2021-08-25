#pragma once

#define MAX_INPUT_KEY 255
#define MAX_INPUT_MOUSE 8

class Keyboard
{
public:
	static Keyboard* Get();

	static void Create();
	static void Delete();

	void Update();

	inline bool Down(DWORD key) { return keyMap[key] == KEY_INPUT_STATUS_DOWN; }
    inline bool Up(DWORD key) { return keyMap[key] == KEY_INPUT_STATUS_UP; }
	inline bool Press(DWORD key) { return keyMap[key] == KEY_INPUT_STATUS_PRESS; }

	inline bool Press(const DWORD* key) { 
		return keyMap[key[0]] == KEY_INPUT_STATUS_PRESS||
			   keyMap[key[1]] == KEY_INPUT_STATUS_PRESS ||
			   keyMap[key[2]] == KEY_INPUT_STATUS_PRESS ||
			   keyMap[key[3]] == KEY_INPUT_STATUS_PRESS ; }

	inline bool Up(const DWORD* key) {
		return keyMap[key[0]] == KEY_INPUT_STATUS_UP ||
			keyMap[key[1]] == KEY_INPUT_STATUS_UP ||
			keyMap[key[2]] == KEY_INPUT_STATUS_UP ||
			keyMap[key[3]] == KEY_INPUT_STATUS_UP;
	}

private:
	Keyboard();
	~Keyboard();

	static Keyboard* instance;

	byte keyState[MAX_INPUT_KEY];
	byte keyOldState[MAX_INPUT_KEY];
	byte keyMap[MAX_INPUT_KEY];

	enum
	{
		KEY_INPUT_STATUS_NONE = 0,
		KEY_INPUT_STATUS_DOWN,
		KEY_INPUT_STATUS_UP,
		KEY_INPUT_STATUS_PRESS,
	};
};