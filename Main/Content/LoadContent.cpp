#include "LoadContent.h"

void LoadContent::OnInit()
{
	ballRight.xPos = 1;					// 1
	ballRight.yPos = 1;					// 1
	ballRight.shape = "��";

	ballLeft.xPos = GAME_SIZE_X - 1;	// 39
	ballLeft.yPos = GAME_SIZE_Y - 2;	// 23
	ballLeft.shape = "��";

	rotateRight = 1;
	rotateLeft = -1;

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
	if (TIMER->GetTickTimer(0.0001f)) {

		curtain[ballRight.yPos][ballRight.xPos] = false;
		curtain[ballLeft.yPos][ballLeft.xPos] = false;

		ballRight.xPos += rotateRight;
		ballLeft.xPos += rotateLeft;

		// ���� ��
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

		// �Ʒ��� ��
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

		if (ballRight.xPos == ballLeft.xPos && ballRight.yPos == ballLeft.yPos)
			SCENE->ChangeContent((int)_ECONTENT::TITLE);

	}
}

void LoadContent::OnRender()
{

	SCREEN->OnDraw(3, 3, "�ε�SCENE ������ �Ϸ��Ͽ����ϴ�.");
	SCREEN->OnDraw(20, 20, "���� �����ͷ� �Ѿ�ſ���");

	for (int y = 0; y < GAME_SIZE_Y; y++) 
	{
		for (int x = 0 ;x < GAME_SIZE_X;x++)
		{
			if (curtain[y][x] == true)
				SCREEN->OnDrawColor(x * 2, y, "��", SKYBLUE);
		}
	}


	SCREEN->OnDrawColor(ballRight.xPos * 2, ballRight.yPos, ballRight.shape.c_str(), PURPLE);
	SCREEN->OnDrawColor(ballLeft.xPos * 2, ballLeft.yPos, ballLeft.shape.c_str(), PURPLE);
}
