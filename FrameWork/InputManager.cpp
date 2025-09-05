#include "InputManager.h"
#include <Windows.h>

void InputManager::OnInit()
{
	for (int i = 0; i < KEYMAX; i++)
	{
		SetKeyDown(i, false);
		SetKeyUp(i, true);
	}
}

void InputManager::OnRelease()
{
}

bool InputManager::OnKeyDown(int key)
{
	if (GetAsyncKeyState(key) & 0x8000)
	{
		if (!this->GetKeyDown()[key])
		{
			this->SetKeyDown(key, true);
			return true;
		}
	}
	else
	{
		this->SetKeyDown(key, false);
	}

	return false;
}

bool InputManager::OnKeyUp(int key)
{
	if (GetAsyncKeyState(key) & 0x8000)
	{
		this->SetKeyUp(key, false);
	}
	else
	{
		if (!this->GetKeyUp()[key])
		{
			this->SetKeyUp(key, true);
			return true;
		}
	}
	return false;
}

bool InputManager::OnKeyStay(int key)
{
	if (GetAsyncKeyState(key) & 0x8001)	return true;
	return false;
}

bool InputManager::OnToggleKey(int key)
{
	if (GetAsyncKeyState(key) & 0x0001) return true;

	return false;
}
