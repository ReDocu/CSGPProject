#include "TitleContent.h"
#include "../../framework.h"

void TitleContent::OnInit()
{
}

void TitleContent::OnRelease()
{
}

void TitleContent::OnUpdate()
{
	if (INPUT->OnKeyDown(VK_RETURN))
		SCENE->ChangeContent((int)_ECONTENT::SNAKE);
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


	SCREEN->OnDraw(10, 14, "타이틀 콘텐츠 입니다. ");
	SCREEN->OnDraw(10, 15, "Enter Key를 누르면 로드 Snake Scene으로 넘어갑니다. ");
}
