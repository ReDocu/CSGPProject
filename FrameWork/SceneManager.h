#pragma once
#include "singleton.h"
#include "Interface/IContent.h"
#include <map>

class SceneManager : public singleton<SceneManager>
{
	std::map<int, IContent*> contentMap;

	IContent* curContent = nullptr;

public:
	void OnInit();
	void OnRelease();

	void AddContent(int key, IContent* content);
	void ChangeContent(int key);

	IContent* GetContent() { return curContent; }

};

