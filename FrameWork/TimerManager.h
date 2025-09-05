#pragma once
#include "singleton.h"
#include <map>

class TimerManager : public singleton<TimerManager>
{
private:

	unsigned long long startTime = 0;	// 프로그램을 시작시킨 시작시간 확인
	unsigned long long contentTime = 0;	// 게임을 시작시킨 시작시작 확인

	std::map<float, unsigned long long> timerMap;	// 시간을 저장해 놓은 map 자료구조 변수

public:
	void OnInit();
	void OnRelease();

	// 타이머 관리 함수
	void AddTimer(float timer);
	void StartContent();

	bool GetTickTimer(float timer);

	float GetProgramTime();
	float GetContentTime();

	

	// 개선이 필요한 상황 frame에 화면 갱신방식
	void SetFrame(float frame);
};

