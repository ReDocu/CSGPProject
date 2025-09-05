#pragma once
#include "../../../FrameWork/Interface/IContent.h"
class IGameContent : public IContent
{
public:
	enum class _EPhase {
		NONE,
		TITLE,
		INGAME
	};

protected:
	_EPhase currentPhase = _EPhase::NONE;
	bool debugMode = false;

public:
	virtual void OnInit();
	virtual void OnRelease();
	virtual void OnUpdate();
	virtual void OnRender();

	virtual void OnTitleUpdate() = 0;
	virtual void OnTitleRender() = 0;

	virtual void OnInGameUpdate() = 0;
	virtual void OnInGameRender() = 0;
};

