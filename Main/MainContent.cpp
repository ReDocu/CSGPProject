#include "MainContent.h"
#include "../framework.h"
#include "../FrameWork/Render/RenderUtil.h"

#include "Content/IntroContent.h"
#include "Content/TitleContent.h"
#include "Content/LoadContent.h"
#include "Content/SnakeContent.h"
#include "Content/TetrisContent.h"
#include "Content/DinoContent.h"
#include "Content/RoadFighterContent.h"
#include "Content/MazeContent.h"
#include "Content/GalagaContent.h"
#include "Content/BattleCityContent.h"
#include "Content/PacManContent.h"
#include "Content/TycoonContent.h"

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
	SCENE->AddContent((int)_ECONTENT::GALAGA, new GalagaContent());
	SCENE->AddContent((int)_ECONTENT::BATTLECITY, new BattleCityContent());
	SCENE->AddContent((int)_ECONTENT::PACMAN, new PacManContent());
	SCENE->AddContent((int)_ECONTENT::TYCOON, new TycoonContent());

	// 콘텐츠를 변경한다.
	SCENE->ChangeContent((int)_ECONTENT::TITLE);
}

void MainContent::OnUpdate()
{
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
	RenderUtil::DrawBorder(GRAY);
}

void MainContent::OnRelease()
{
}
