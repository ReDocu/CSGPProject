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

		// Update 내용
		main->OnUpdate();

		// ESC: scene did not consume it (e.g. title) -> quit program
		if (INPUT->OnKeyDown(VK_ESCAPE))
		{
			break;
		}

		// title menu EXIT -> quit
		if (SCENE->IsQuitRequested())
		{
			break;
		}

		// Render 내용
		main->OnRender();

		SCREEN->FlippingBuffer();

		// 1초에 60 프레임
		TIMER->SetFrame(1000.0f / 60.0f);
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
