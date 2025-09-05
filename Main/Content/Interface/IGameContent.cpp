#include "IGameContent.h"
#include "../../../framework.h"

void IGameContent::OnInit()
{
	currentPhase = _EPhase::TITLE;


	TIMER->StartContent();
}

void IGameContent::OnRelease()
{
}

void IGameContent::OnUpdate()
{
	switch (currentPhase)
	{
	case IGameContent::_EPhase::NONE:
		break;
	case IGameContent::_EPhase::TITLE:
		OnTitleUpdate();
		break;
	case IGameContent::_EPhase::INGAME:
		OnInGameUpdate();
		break;
	default:
		break;
	}
}

void IGameContent::OnRender()
{
	switch (currentPhase)
	{
	case IGameContent::_EPhase::NONE:
		break;
	case IGameContent::_EPhase::TITLE:
		OnTitleRender();
		break;
	case IGameContent::_EPhase::INGAME:
		OnInGameRender();
		break;
	default:
		break;
	}

}
