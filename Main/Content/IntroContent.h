#pragma once
#include "../../FrameWork/Interface/IContent.h"

class IntroContent : public IContent
{
public:
	virtual void OnInit();
	virtual void OnRelease();
	virtual void OnUpdate();
	virtual void OnRender();
};

