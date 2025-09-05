#pragma once

// 공용으로 활용되는 헤더
#include <Windows.h>
#include <time.h>
#include <stdlib.h>
#include <string>

// 싱글톤으로 제작된 헤더
#include "FrameWork/ScreenManager.h"
#include "FrameWork/TimerManager.h"
#include "FrameWork/InputManager.h"
#include "FrameWork/SceneManager.h"

// 싱글톤 접근 매크로
#define SCREEN ScreenManager::GetSingleton()
#define TIMER TimerManager::GetSingleton()
#define INPUT InputManager::GetSingleton()
#define SCENE SceneManager::GetSingleton()

// 게임 화면 사이즈 조정
// 해상도 40X25  [영문의 경우 80X25]
#define GAME_SIZE_X 40	// 한글은 2Byte 영어는 1Byte이므로 영문으로 할 경우 80으로 조정한다.
#define GAME_SIZE_Y 25	//

// 색깔 조정
#define BLACK		0
#define DARKBLUE	1
#define DARKGREEN	2
#define DARKSKYBLUE 3
#define DARKRED		4
#define DARKPURPLE	5
#define DARKYELLOW	6
#define GRAY		7
#define DARKGRAY	8
#define BLUE		9
#define GREEN		10
#define SKYBLUE		11
#define RED			12
#define PURPLE		13
#define YELLOW 		14
#define WHITE		15

enum class _ECONTENT {
	INTRO,
	TITLE,
	LOAD,
	SNAKE
};