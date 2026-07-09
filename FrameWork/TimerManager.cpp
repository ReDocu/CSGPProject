#include "TimerManager.h"
#include <Windows.h>

void TimerManager::OnInit()
{
	startTime = GetTickCount64();
	contentTime = GetTickCount64();
	frameTime = GetTickCount64();
}

void TimerManager::OnRelease()
{
	timerMap.clear();
}

void TimerManager::AddTimer(float timer)
{
	if (timerMap.find(timer) == timerMap.end())
		timerMap.insert(std::make_pair(timer, GetTickCount64()));
}

void TimerManager::StartContent()
{
	contentTime = GetTickCount64();
}

bool TimerManager::GetTickTimer(float timer)
{
	int check = 0;
	if (timerMap.find(timer) != timerMap.end())
	{
		if ((double)(GetTickCount64() - timerMap[timer]) / 1000.0 > timer)
		{
			timerMap[timer] = GetTickCount64();
			return true;
		}
		return false;
	}
	else
	{
		AddTimer(timer);
		return false;
	}
}

float TimerManager::GetProgramTime()
{
	return (float)(GetTickCount64() - startTime) / 1000.0f;
}

float TimerManager::GetContentTime()
{
	return (float)(GetTickCount64() - contentTime) / 1000.0f;
}

void TimerManager::SetFrame(float frame)
{
	unsigned long long elapsed = GetTickCount64() - frameTime;

	if ((float)elapsed < frame)
		Sleep((DWORD)(frame - (float)elapsed));

	frameTime = GetTickCount64();
}
