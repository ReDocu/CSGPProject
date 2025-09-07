# CSGP Project

## C++ 터미널 콘솔 게임 개발 프레임워크

- Input 기능 구현
- Scene 관리 기능 구현
- Screen 구현
- Timer 구현

---

## Life Cycle

```c
MainContent* main = new MainContent();
	
bool gameState = true;	
main->OnInit();
while (gameState)
{
	SCREEN->ClearBuffer();
	if (INPUT->OnKeyDown(VK_ESCAPE))
	{
		break;
	}
	// Update 
	main->OnUpdate();

	// Render 
	main->OnRender();
	SCREEN->FlippingBuffer();

	// Tick Timer
	TIMER->SetFrame(1000 / 60);
}

main->OnRelease();
delete main;
```

## Scene 추가 방법

IContent [순수가상함수] - IGameContent 상속 받아 사용

```c
void MainContent::OnInit()
{
	// _ECONTENT
	SCENE->AddContent((int)_ECONTENT::INTRO, new IntroContent());
	SCENE->AddContent((int)_ECONTENT::TITLE, new TitleContent());
	SCENE->AddContent((int)_ECONTENT::LOAD, new LoadContent());
	SCENE->AddContent((int)_ECONTENT::SNAKE, new SnakeContent());

	// 
	SCENE->ChangeContent((int)_ECONTENT::SNAKE);
}
```

## 키 사용법

```c
if(INPUT->OnKeyDown('Z'))
	SCENE->ChangeContent((int)_ECONTENT::INTRO);
if(INPUT->OnKeyDown('X'))
	SCENE->ChangeContent((int)_ECONTENT::TITLE);
```

## 예제는 SnakeContent 참고