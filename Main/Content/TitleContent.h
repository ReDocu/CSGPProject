#pragma once
#include "../../FrameWork/Interface/IContent.h"

class TitleContent : public IContent
{
public:
	virtual void OnInit();
	virtual void OnRelease();
	virtual void OnUpdate();
	virtual void OnRender();
};

