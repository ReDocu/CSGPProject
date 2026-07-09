#include "MainContent.h"
#include "../framework.h"

#include "Content/IntroContent.h"
#include "Content/TitleContent.h"
#include "Content/LoadContent.h"
#include "Content/SnakeContent.h"
#include "Content/TetrisContent.h"
#include "Content/DinoContent.h"
#include "Content/RoadFighterContent.h"
#include "Content/MazeContent.h"

void MainContent::OnInit()
{
	// _ECONTENT와 실제 클래스를 연결시켜준다.
	SCENE->AddContent((int)_ECONTENT::INTRO, new IntroContent());
	SCENE->AddContent((int)_ECONTENT::TITLE, new TitleContent());
	SCENE->AddContent((int)_ECONTENT::LOAD, new LoadContent());
	SCENE->AddContent((int)_ECONTENT::SNAKE, new SnakeContent());
	SCENE->AddContent((int)_ECONTENT::TETRIS, new TetrisContent());
	SCENE->AddContent((int)_ECONTENT::DINO, new DinoContent());
	SCENE->AddContent((int)_ECONTENT::ROADFIGHTER, new RoadFighterContent());
	SCENE->AddContent((int)_ECONTENT::MAZE, new MazeContent());

	// 콘텐츠를 변경한다.
	SCENE->ChangeContent((int)_ECONTENT::MAZE);
}

void MainContent::OnUpdate()
{
	// 변경을 확인하기 위한 키 입력 
	//if(INPUT->OnKeyDown('Z'))
	//	SCENE->ChangeContent((int)_ECONTENT::INTRO);
	//if(INPUT->OnKeyDown('X'))
	//	SCENE->ChangeContent((int)_ECONTENT::TETRIS);


	// 현재 실행중인 컨텐츠의 업데이트를 불러온다.
	if (SCENE->GetContent() != nullptr)
		SCENE->GetContent()->OnUpdate();
}

void MainContent::OnRender()
{

	// 현재 실행중인 컨텐츠의 렌더를 불러온다.
	if (SCENE->GetContent() != nullptr)
		SCENE->GetContent()->OnRender();

	// 배경
	for (short y = 0; y < GAME_SIZE_Y; y++)
	{
		for (short x = 0; x < GAME_SIZE_X; x++)
		{
			if (x == 0 || y == 0 || x == GAME_SIZE_X - 1 || y == GAME_SIZE_Y - 1)
				SCREEN->OnDrawColor(x * 2, y, "■", GRAY);
		}
	}
}

void MainContent::OnRelease()
{
}
