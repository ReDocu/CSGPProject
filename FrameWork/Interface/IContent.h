#pragma once

class IContent abstract
{
public:
	virtual void OnInit() = 0;
	virtual void OnRelease() = 0;
	virtual void OnUpdate() = 0;
	virtual void OnRender() = 0;
};