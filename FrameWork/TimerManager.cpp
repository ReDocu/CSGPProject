#include "TimerManager.h"
#include <Windows.h>
#include <time.h>

void TimerManager::OnInit()
{
	startTime = GetTickCount64();
	contentTime = GetTickCount64();
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
		if ((double)(GetTickCount64() - timerMap[timer]) / CLOCKS_PER_SEC > timer)
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
	return (float)(GetTickCount64() - startTime) / CLOCKS_PER_SEC;
}

float TimerManager::GetContentTime()
{
	return (float)(GetTickCount64() - contentTime) / CLOCKS_PER_SEC;
}

void TimerManager::SetFrame(float frame)
{
	
	unsigned long long start = GetTickCount64();

	while (true)
	{
		if (GetTickCount64() - start >= frame)
			return;
	}
}
