#include "LoadContent.h"

// destination name for the loading banner
static const char* SceneName(int key)
{
	switch ((_ECONTENT)key)
	{
	case _ECONTENT::SNAKE:       return "S N A K E";
	case _ECONTENT::TETRIS:      return "T E T R I S";
	case _ECONTENT::DINO:        return "D I N O";
	case _ECONTENT::ROADFIGHTER: return "R O A D  F I G H T E R";
	case _ECONTENT::MAZE:        return "M A Z E";
	case _ECONTENT::GALAGA:      return "G A L A G A";
	case _ECONTENT::BATTLECITY:  return "B A T T L E  C I T Y";
	case _ECONTENT::PACMAN:      return "P A C - M A N";
	case _ECONTENT::TYCOON:      return "G A M E  D E V  T Y C O O N";
	default:                     return "M A I N  M E N U";
	}
}

void LoadContent::OnInit()
{
	ballRight.xPos = 1;					// 1
	ballRight.yPos = 1;					// 1
	ballRight.shape = "●";

	ballLeft.xPos = GAME_SIZE_X - 1;	// 39
	ballLeft.yPos = GAME_SIZE_Y - 2;	// 23
	ballLeft.shape = "●";

	rotateRight = 1;
	rotateLeft = -1;

	// banner: where are we going after loading?
	targetName = SceneName(SCENE->PeekReservedContent());

	for (int y = 0; y < GAME_SIZE_Y; y++)
	{
		for (int x = 0; x < GAME_SIZE_X; x++)
		{
			curtain[y][x] = true;
		}
	}

}

void LoadContent::OnRelease()
{
	
}

void LoadContent::OnUpdate()
{
	// ESC: skip the loading animation (consume so main loop does not quit)
	if (INPUT->OnKeyDown(VK_ESCAPE))
	{
		int next = SCENE->PopReservedContent();
		SCENE->ChangeContent(next >= 0 ? next : (int)_ECONTENT::TITLE);
		return;
	}

	for (int step = 0; step < 8; step++) {

		curtain[ballRight.yPos][ballRight.xPos] = false;
		curtain[ballLeft.yPos][ballLeft.xPos] = false;

		ballRight.xPos += rotateRight;
		ballLeft.xPos += rotateLeft;

		// 위쪽 공
		if (ballRight.xPos > GAME_SIZE_X - 1)
		{
			ballRight.xPos = GAME_SIZE_X - 1;
			ballRight.yPos++;
			rotateRight *= -1;
		}

		if (ballRight.xPos < 1)
		{
			ballRight.xPos = 1;
			ballRight.yPos++;
			rotateRight *= -1;
		}

		// 아래쪽 공
		if (ballLeft.xPos > GAME_SIZE_X - 1)
		{
			ballLeft.xPos = GAME_SIZE_X - 1;
			ballLeft.yPos--;
			rotateLeft *= -1;
		}

		if (ballLeft.xPos < 1)
		{
			ballLeft.xPos = 1;
			ballLeft.yPos--;
			rotateLeft *= -1;
		}

		if ((ballRight.xPos == ballLeft.xPos && ballRight.yPos == ballLeft.yPos) ||
			ballRight.yPos > ballLeft.yPos)
		{
			int next = SCENE->PopReservedContent();
			SCENE->ChangeContent(next >= 0 ? next : (int)_ECONTENT::TITLE);
			return;
		}

	}
}

void LoadContent::OnRender()
{
	for (int y = 0; y < GAME_SIZE_Y; y++) 
	{
		for (int x = 0 ;x < GAME_SIZE_X;x++)
		{
			if (curtain[y][x] == true)
				SCREEN->OnDrawColor(x * 2, y, "■", SKYBLUE);
		}
	}


	SCREEN->OnDrawColor(ballRight.xPos * 2, ballRight.yPos, ballRight.shape.c_str(), PURPLE);
	SCREEN->OnDrawColor(ballLeft.xPos * 2, ballLeft.yPos, ballLeft.shape.c_str(), PURPLE);

	// loading banner
	SCREEN->OnDrawColor(30, 10, "N O W   L O A D I N G", GRAY);
	SCREEN->OnDrawColor((short)(40 - (int)targetName.size() / 2), 13, targetName.c_str(), YELLOW);
}
