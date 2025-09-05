#include "IntroContent.h"
#include "../../framework.h"

void IntroContent::OnInit()
{
}

void IntroContent::OnRelease()
{
}

void IntroContent::OnUpdate()
{
}

void IntroContent::OnRender()
{
	SCREEN->OnDraw(10, 10, "인트로 콘텐츠");
}
