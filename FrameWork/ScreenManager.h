#pragma once
#include "singleton.h"
#include <Windows.h>

class ScreenManager : public singleton<ScreenManager>
{
#define SCREEN_SIZE_X 80
#define SCREEN_SIZE_Y 50

	unsigned short defaultTextColor = 0;
	unsigned short defaultBGColor = 0;

	HANDLE hBuffer[2];		// 스크린 화면을 담당하는 버퍼 [2개를 준비한다]
	short nScreenIndex;		// 2개의 버퍼를 전환할 수 있는 index를 준비한다.

public:
	void OnInit();
	void OnRelease();

	// 출력 함수
	void OnDraw(short x, short y, const char* msg);
	void OnDrawColor(short x, short y, const char* msg, unsigned short color, unsigned short bgColor = 255);


	// 더블버퍼링
	void CreateBuffer();		// 버퍼(스크린)를 생성한다.
	void ClearBuffer();			// 버퍼를 갱신한다.
	void FlippingBuffer();		// 버프를 전환한다.
	void DeleteBuffer();		// 버퍼(스크린)를 삭제한다.


	// 학습용 함수
	void gotoxy(short x, short y);
	void SetColor(unsigned short color, unsigned short bgColor);
};