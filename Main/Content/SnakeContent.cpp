#include "SnakeContent.h"
#include "../../framework.h"

void SnakeContent::OnInit()
{
	IGameContent::OnInit();

	currentPhase = _EPhase::TITLE;
	direct = _EDIRECT::NONE;

	select = _ESELECT::START;

	SnakeInit();
	ItemInit(10);
	TrapInit(10);
	TunnelInit(6);

	stage = 1;
	score = 0;
	life = 5;
	userName = "NoName";
}

void SnakeContent::OnRelease()
{
	IGameContent::OnRelease();

	for (snakeIter = snake.begin(); snakeIter != snake.end(); snakeIter++)
	{
		delete (*snakeIter);
		(*snakeIter) = nullptr;
	}
	snake.clear();

	for (itemListIter = itemList.begin(); itemListIter != itemList.end(); itemListIter++)
	{
		delete (*itemListIter);
		(*itemListIter) = nullptr;
	}
	itemList.clear();

	for (trapListIter = trapList.begin(); trapListIter != trapList.end(); trapListIter++)
	{
		delete (*trapListIter);
		(*trapListIter) = nullptr;
	}
	trapList.clear();

	for (tunnelListIter = tunnelList.begin(); tunnelListIter != tunnelList.end(); tunnelListIter++)
	{
		(*tunnelListIter)->DestroyTunnel();
		delete (*tunnelListIter);
		(*tunnelListIter) = nullptr;
	}
	tunnelList.clear();
}

void SnakeContent::OnTitleUpdate()
{

	if (INPUT->OnKeyDown(VK_DOWN))
	{
		select = (_ESELECT)(((int)select) + 1);
		
		if (select == _ESELECT::END)
			select = _ESELECT::START;
		
	}
	
	if (INPUT->OnKeyDown(VK_UP))
	{
		select = (_ESELECT)(((int)select) - 1);

		if (select == _ESELECT::NONE)
			select = _ESELECT::EXIT;
	}


	if (INPUT->OnKeyDown(VK_RETURN))
	{
		switch (select)
		{
		case SnakeContent::_ESELECT::START:
			currentPhase = _EPhase::INGAME;
			break;
		case SnakeContent::_ESELECT::EXIT:
			SCENE->ChangeContent((int)_ECONTENT::TITLE);
			break;
		default:
			break;
		}
	}
}

void SnakeContent::OnTitleRender()
{
	SCREEN->OnDraw(11, 3, "■■■■■  ■      ■      ■      ■    ■■  ■■■■■");
	SCREEN->OnDraw(11, 4, "■          ■    ■■    ■  ■    ■  ■      ■        ");
	SCREEN->OnDraw(11, 5, "■■■■■  ■  ■  ■  ■■■■■  ■■        ■■■■■");
	SCREEN->OnDraw(11, 6, "        ■  ■■    ■  ■      ■  ■  ■      ■        ");
	SCREEN->OnDraw(11, 7, "■■■■■  ■      ■  ■      ■  ■    ■■  ■■■■■");

	switch (select)
	{
	case SnakeContent::_ESELECT::START:
		SCREEN->OnDrawColor(11, 12, "▶", RED);
		break;
	case SnakeContent::_ESELECT::EXIT:
		SCREEN->OnDrawColor(11, 15, "▶", RED);
		break;
	default:
		break;
	}

	SCREEN->OnDrawColor(18, 12, "게 임 시 작", BLUE);
	SCREEN->OnDrawColor(18, 15, "게 임 종 료", BLUE);
}

void SnakeContent::OnInGameUpdate()
{
	if (INPUT->OnKeyDown(VK_SPACE))
		currentPhase = _EPhase::TITLE;

	if (INPUT->OnKeyDown(VK_RIGHT))
	{
		direct = _EDIRECT::RIGHT;
	}
	if (INPUT->OnKeyDown(VK_LEFT))
	{
		direct = _EDIRECT::LEFT;
	}
	if (INPUT->OnKeyDown(VK_UP))
	{
		direct = _EDIRECT::UP;
	}
	if (INPUT->OnKeyDown(VK_DOWN))
	{
		direct = _EDIRECT::DOWN;
	}

	if (TIMER->GetTickTimer(0.1f))
	{
		int xPos = 0;
		int yPos = 0;
		int saveXPos = 0;
		int saveYPos = 0;
		for (snakeIter = snake.begin(); snakeIter != snake.end(); snakeIter++)
		{

			if (snakeIter == snake.begin())
			{
				saveXPos = (*snakeIter)->xPos;
				saveYPos = (*snakeIter)->yPos;
				continue;
			}

			xPos = saveXPos;
			yPos = saveYPos;

			saveXPos = (*snakeIter)->xPos;
			saveYPos = (*snakeIter)->yPos;

			(*snakeIter)->xPos = xPos;
			(*snakeIter)->yPos = yPos;
		}

		// 움직임 제어
		MoveSnake();

		for (itemListIter = itemList.begin(); itemListIter != itemList.end(); itemListIter++)
		{
			if ((*itemListIter)->xPos == (*snake.begin())->xPos &&
				(*itemListIter)->yPos == (*snake.begin())->yPos)
			{
				MoveItem(*itemListIter);
				snake.push_back(new Snake(snake.back()->xPos, snake.back()->yPos, "■", DARKGREEN));
				score += 10;
				
			}
		}

		for (trapListIter = trapList.begin(); trapListIter != trapList.end(); trapListIter++)
		{
			if ((*trapListIter)->xPos == (*snake.begin())->xPos &&
				(*trapListIter)->yPos == (*snake.begin())->yPos)
			{
				SnakeInit();
				life--;
			}
		}


		for (tunnelListIter = tunnelList.begin(); tunnelListIter != tunnelList.end(); tunnelListIter++)
		{
			// start 지점 충돌 체크
			if ((*tunnelListIter)->startTunnel->xPos == (*snake.begin())->xPos &&
				(*tunnelListIter)->startTunnel->yPos == (*snake.begin())->yPos)
			{
				(*snake.begin())->xPos = (*tunnelListIter)->endTunnel->xPos;
				(*snake.begin())->yPos = (*tunnelListIter)->endTunnel->yPos;

				MoveSnake();
			}

			// end 지점 충돌 체크
			if ((*tunnelListIter)->endTunnel->xPos == (*snake.begin())->xPos &&
				(*tunnelListIter)->endTunnel->yPos == (*snake.begin())->yPos)
			{
				(*snake.begin())->xPos = (*tunnelListIter)->startTunnel->xPos;
				(*snake.begin())->yPos = (*tunnelListIter)->startTunnel->yPos;

				MoveSnake();
			}
		}


		if (
			(*snake.begin())->xPos <= wallPosX || (*snake.begin())->yPos <= wallPosY || 
			(*snake.begin())->xPos >= WALL_X - 1 - wallPosX ||
			(*snake.begin())->yPos >= WALL_Y - 1 - wallPosY)
		{
			SnakeInit();
			life--;
		}

	}

	if (life == 0)
	{
		currentPhase = _EPhase::TITLE;
	}

}

void SnakeContent::OnInGameRender()
{

	int uiBox_xPos = WALL_X * 2;
	int uiBox_yPos = 1;

	SCREEN->OnDrawColor(uiBox_xPos + 6, uiBox_yPos + 1, "S N A K E   G A M E", SKYBLUE);
	
	SCREEN->OnDrawColor(uiBox_xPos + 2, uiBox_yPos + 3, "SCORE : ", SKYBLUE);
	SCREEN->OnDrawColor(uiBox_xPos + 12, uiBox_yPos + 3, std::to_string(score).c_str(), SKYBLUE);
	
	SCREEN->OnDrawColor(uiBox_xPos + 2, uiBox_yPos + 5, "STAGE : ", SKYBLUE);
	SCREEN->OnDrawColor(uiBox_xPos + 12, uiBox_yPos + 5, std::to_string(stage).c_str(), SKYBLUE);

	SCREEN->OnDrawColor(uiBox_xPos + 2, uiBox_yPos + 7, "LIFE : ", SKYBLUE);
	SCREEN->OnDrawColor(uiBox_xPos + 12, uiBox_yPos + 7, std::to_string(life).c_str(), SKYBLUE);

	SCREEN->OnDrawColor(uiBox_xPos + 2, uiBox_yPos + 19, "GAME TIME : ", SKYBLUE);
	SCREEN->OnDrawColor(uiBox_xPos + 15, uiBox_yPos + 19, std::to_string((int)TIMER->GetContentTime()).c_str(), SKYBLUE);

	SCREEN->OnDrawColor(uiBox_xPos + 2, uiBox_yPos + 21, "PLAY TIME : ", SKYBLUE);
	SCREEN->OnDrawColor(uiBox_xPos + 15, uiBox_yPos + 21, std::to_string((int)TIMER->GetProgramTime()).c_str(), SKYBLUE);

	for (snakeIter = snake.begin(); snakeIter != snake.end(); snakeIter++)
	{
		SCREEN->OnDrawColor((*snakeIter)->xPos * 2, (*snakeIter)->yPos, (*snakeIter)->shape.c_str(), (*snakeIter)->color);
	}

	for (itemListIter = itemList.begin(); itemListIter != itemList.end(); itemListIter++)
	{
		SCREEN->OnDrawColor((*itemListIter)->xPos * 2, (*itemListIter)->yPos, (*itemListIter)->shape.c_str(), (*itemListIter)->color);
	}

	for (trapListIter = trapList.begin(); trapListIter != trapList.end(); trapListIter++)
	{
		SCREEN->OnDrawColor((*trapListIter)->xPos * 2, (*trapListIter)->yPos, (*trapListIter)->shape.c_str(), (*trapListIter)->color);
	}

	for (tunnelListIter = tunnelList.begin(); tunnelListIter != tunnelList.end(); tunnelListIter++)
	{
		SCREEN->OnDrawColor((*tunnelListIter)->startTunnel->xPos * 2, (*tunnelListIter)->startTunnel->yPos, (*tunnelListIter)->startTunnel->shape.c_str(), (*tunnelListIter)->startTunnel->color);
		SCREEN->OnDrawColor((*tunnelListIter)->endTunnel->xPos * 2, (*tunnelListIter)->endTunnel->yPos, (*tunnelListIter)->endTunnel->shape.c_str(), (*tunnelListIter)->endTunnel->color);
	}

	for (int y = wallPosY; y < WALL_Y - 1; y++)
	{
		for (int x = wallPosX; x < WALL_X - 1; x++)
		{
			if (x == wallPosX || y == wallPosY || x == WALL_X - 1 - wallPosX || y == WALL_Y - 1 - wallPosY)
			{
				SCREEN->OnDrawColor(x * 2, y, "■", DARKGRAY);
			}
		}
	}
}

void SnakeContent::SnakeInit()
{
	for (snakeIter = snake.begin(); snakeIter != snake.end(); snakeIter++)
	{
		delete (*snakeIter);
		(*snakeIter) = nullptr;
	}
	snake.clear();

	Snake* head = new Snake(5, 5, "■", GREEN);
	snake.push_back(head);

	direct = _EDIRECT::NONE;
}

void SnakeContent::ItemInit(int itemCount)
{
	for (itemListIter = itemList.begin(); itemListIter != itemList.end(); itemListIter++)
	{
		delete (*itemListIter);
		(*itemListIter) = nullptr;
	}
	itemList.clear();

	for (int i = 0;i < itemCount;i++)
	{
		int xPos = 2 + rand()% 20;
		int yPos = 2 + rand() % 20;

		Item* newItem = new Item(xPos, yPos, "★", RED);
		MoveItem(newItem);
		itemList.push_back(newItem);
	}
}

void SnakeContent::TrapInit(int trapCount)
{
	for (trapListIter = trapList.begin(); trapListIter != trapList.end(); trapListIter++)
	{
		delete (*trapListIter);
		(*trapListIter) = nullptr;
	}
	trapList.clear();

	for (int i = 0; i < trapCount; i++)
	{
		int xPos = 2 + rand() % 20;
		int yPos = 2 + rand() % 20;

		Trap* newTrap = new Trap(xPos, yPos, "♠", DARKBLUE);
		MoveTrap(newTrap);
		trapList.push_back(newTrap);
	}
}

// 5개 이상은 생성하지 마시오
void SnakeContent::TunnelInit(int tunnelCount)
{
	for (tunnelListIter = tunnelList.begin(); tunnelListIter != tunnelList.end(); tunnelListIter++)
	{
		(*tunnelListIter)->DestroyTunnel();
		delete (*tunnelListIter);
		(*tunnelListIter) = nullptr;
	}
	tunnelList.clear();

	for (int i = 0;i < tunnelCount;i++)
	{
		int startXPos = 2 + rand() % 20;
		int startYPos = 2 + rand() % 20;
		int endXPos = 2 + rand() % 20;
		int endYPos = 2 + rand() % 20;

		while (startXPos == endXPos && startYPos == endYPos)
		{
			startXPos = 2 + rand() % 20;
			startYPos = 2 + rand() % 20;
			endXPos = 2 + rand() % 20;
			endYPos = 2 + rand() % 20;
		}

		Tunnel* startTunnel = new Tunnel(startXPos, startYPos, "□", 9 + i);
		Tunnel* endTunnel = new Tunnel(endXPos, endYPos, "□", 9 + i);

		TunnelConnect* newTunnel = new TunnelConnect(startTunnel, endTunnel);

		tunnelList.push_back(newTunnel);
	}
}

void SnakeContent::MoveSnake()
{
	// 움직임 제어
	switch (direct)
	{
	case SnakeContent::_EDIRECT::NONE:
		break;
	case SnakeContent::_EDIRECT::RIGHT:
		(*snake.begin())->xPos++;
		break;
	case SnakeContent::_EDIRECT::LEFT:
		(*snake.begin())->xPos--;
		break;
	case SnakeContent::_EDIRECT::UP:
		(*snake.begin())->yPos--;
		break;
	case SnakeContent::_EDIRECT::DOWN:
		(*snake.begin())->yPos++;
		break;
	default:
		break;
	}
}

void SnakeContent::MoveItem(Item* item)
{
	item->xPos = 2 + rand() % 20;
	item->yPos = 2 + rand() % 20;

	for (trapListIter = trapList.begin(); trapListIter != trapList.end(); trapListIter++)
	{
		while (item->xPos == (*trapListIter)->xPos && item->yPos == (*trapListIter)->yPos)
		{
			item->xPos = 2 + rand() % 20;
			item->yPos = 2 + rand() % 20;
		}
	}
}

void SnakeContent::MoveTrap(Trap* trap)
{
	trap->xPos = 2 + rand() % 20;
	trap->yPos = 2 + rand() % 20;

	// 아이템과 충돌 체크
	for (itemListIter = itemList.begin(); itemListIter != itemList.end(); itemListIter++)
	{
		while (trap->xPos == (*itemListIter)->xPos && trap->yPos == (*itemListIter)->yPos)
		{
			trap->xPos = 2 + rand() % 20;
			trap->yPos = 2 + rand() % 20;
		}
	}
}
