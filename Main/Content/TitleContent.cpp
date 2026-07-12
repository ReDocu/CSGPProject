#include "TitleContent.h"
#include "../../framework.h"

void TitleContent::OnInit()
{
}

void TitleContent::OnRelease()
{
}

static int s_sel = 0;	// 0~8: 게임, 9: EXIT

static const _ECONTENT MENU_SCENE[9] = {
	_ECONTENT::TYCOON, _ECONTENT::SNAKE, _ECONTENT::TETRIS,
	_ECONTENT::DINO, _ECONTENT::ROADFIGHTER, _ECONTENT::MAZE,
	_ECONTENT::GALAGA, _ECONTENT::BATTLECITY, _ECONTENT::PACMAN
};

void TitleContent::OnUpdate()
{
	if (INPUT->OnKeyDown(VK_UP))
		s_sel = (s_sel + 9) % 10;
	if (INPUT->OnKeyDown(VK_DOWN))
		s_sel = (s_sel + 1) % 10;
	if (INPUT->OnKeyDown(VK_LEFT) || INPUT->OnKeyDown(VK_RIGHT))
		s_sel = (s_sel + 5) % 10;

	if (INPUT->OnKeyDown(VK_RETURN))
	{
		if (s_sel < 9)
			SCENE->ChangeContentWithLoading((int)_ECONTENT::LOAD, (int)MENU_SCENE[s_sel]);
		else
			SCENE->RequestQuit();
	}
}

void TitleContent::OnRender()
{
	short startPos = 16;

	SCREEN->OnDraw(startPos, 3, "■■■■■  ■■■■■  ■■■■■  ■■■■■  ");
	SCREEN->OnDraw(startPos, 4, "■          ■          ■          ■      ■  ");
	SCREEN->OnDraw(startPos, 5, "■          ■          ■          ■      ■  ");
	SCREEN->OnDraw(startPos, 6, "■          ■■■■■  ■  ■■■  ■■■■■  ");
	SCREEN->OnDraw(startPos, 7, "■                  ■  ■  ■  ■  ■          ");
	SCREEN->OnDraw(startPos, 8, "■                  ■  ■      ■  ■          ");
	SCREEN->OnDraw(startPos, 9, "■■■■■  ■■■■■  ■■■■■  ■          ");

	SCREEN->OnDraw(27, 11, "Console Game Pack ver 0.2");

	static const char* MENU_NAME[10] = {
		"1. GameDev Tycoon", "2. Snake", "3. Tetris", "4. Dino", "5. RoadFighter",
		"6. Maze", "7. Galaga", "8. BattleCity", "9. PacMan", "EXIT"
	};

	for (int i = 0; i < 10; i++)
	{
		short x = (i < 5) ? 16: 40;
		short y = (short)(14 + (i % 5));
		SCREEN->OnDrawColor(x, y, MENU_NAME[i], BLUE);
	}
	SCREEN->OnDrawColor(s_sel < 5 ? 14 : 38, (short)(14 + (s_sel % 5)), "▶", RED);

	SCREEN->OnDrawColor(14, 21, "방향키: 이동   Enter: 선택   숫자키: 바로 이동   ESC: 종료", GRAY);
}
