#pragma once
#include "singleton.h"
#include <bitset>

class InputManager : public singleton<InputManager>
{
#define KEYMAX 256

private:
	std::bitset<KEYMAX> keyUp;
	std::bitset<KEYMAX> keyDown;

public:
	void OnInit();
	void OnRelease();

	// 사용하는 기능
	bool OnKeyDown(int key);
	bool OnKeyUp(int key);
	bool OnKeyStay(int key);
	bool OnToggleKey(int key);


	// Getter Setter
	std::bitset<KEYMAX>	GetKeyUp() { return keyUp; }
	std::bitset<KEYMAX>	GetKeyDown() { return keyDown; }

	void SetKeyUp(int key, bool state) { keyUp.set(key, state); }
	void SetKeyDown(int key, bool state) { keyDown.set(key, state); }
};

