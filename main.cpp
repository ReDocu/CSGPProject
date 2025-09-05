#include "Main/MainContent.h"
#include "framework.h"

int main() {

	srand(time(NULL));

	SCREEN->OnInit();
	TIMER->OnInit();
	INPUT->OnInit();
	SCENE->OnInit();

	MainContent* main = new MainContent();
	main->OnInit();

	bool gameState = true;	// 추후 변경될 사항

	while (gameState)
	{
		SCREEN->ClearBuffer();

		if (INPUT->OnKeyDown(VK_ESCAPE))
		{
			break;
		}

		// Update 내용
		main->OnUpdate();

		// Render 내용
		main->OnRender();

		SCREEN->FlippingBuffer();

		// 1초에 60 프레임
		TIMER->SetFrame(1000 / 60);
	}
	main->OnRelease();
	delete main;

	SCENE->OnRelease();
	SCENE->ReleaseSingleton();

	INPUT->OnRelease();
	INPUT->ReleaseSingleton();

	TIMER->OnRelease();
	TIMER->ReleaseSingleton();

	SCREEN->OnRelease();
	SCREEN->ReleaseSingleton();


	return 0;
}
