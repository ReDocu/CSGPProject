#pragma once
#include "singleton.h"
#include "Interface/IContent.h"
#include <map>

class SceneManager : public singleton<SceneManager>
{
	std::map<int, IContent*> contentMap;

	IContent* curContent = nullptr;

	bool quitRequest = false;

	int reservedContent = -1;	// scene to enter after the loading screen (-1 = none)

public:
	void OnInit();
	void OnRelease();

	void AddContent(int key, IContent* content);
	void ChangeContent(int key);

	IContent* GetContent() { return curContent; }

	// menu-driven program exit (main loop polls this)
	void RequestQuit() { quitRequest = true; }
	bool IsQuitRequested() { return quitRequest; }

	// loading-screen transition: A -> loadKey -> key
	// the loading scene pops the reservation when its animation ends
	void ReserveContent(int key) { reservedContent = key; }
	int  PeekReservedContent() { return reservedContent; }
	int  PopReservedContent() { int k = reservedContent; reservedContent = -1; return k; }
	void ChangeContentWithLoading(int loadKey, int key)
	{
		ReserveContent(key);
		ChangeContent(loadKey);
	}

};

