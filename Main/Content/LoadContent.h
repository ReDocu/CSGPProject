#pragma once
#include "../../FrameWork/Interface/IContent.h"

#include "../../framework.h"

#include <string>

class LoadContent : public IContent
{
	struct Ball {
		int xPos;
		int yPos;
		std::string shape;
	};

	Ball ballRight;
	Ball ballLeft;

	int rotateRight;
	int rotateLeft;

	bool curtain[GAME_SIZE_Y][GAME_SIZE_X]{ false };



public:
	virtual void OnInit();
	virtual void OnRelease();
	virtual void OnUpdate();
	virtual void OnRender();
};

