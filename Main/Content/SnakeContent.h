#pragma once
#include "Interface/IGameContent.h"
#include <string>
#include <vector>

class SnakeContent : public IGameContent
{
private:

#define WALL_X 25
#define WALL_Y 25

	enum class _ESELECT {
		NONE,
		START,
		EXIT,
		END,
	};

	enum class _EDIRECT {
		NONE,
		RIGHT,
		LEFT,
		UP,
		DOWN
	};

	class Snake {
	public:
		int xPos;
		int yPos;
		std::string shape;
		unsigned short color;
		
	public:
		Snake(int xPos, int yPos, std::string shape, unsigned short color) : xPos(xPos), yPos(yPos), shape(shape), color(color) {}
	};

	class Item {
	public:
		int xPos;
		int yPos;
		std::string shape;
		unsigned short color;

	public:
		Item(int xPos, int yPos, std::string shape, unsigned short color) : xPos(xPos), yPos(yPos), shape(shape), color(color) {}
	};

	class Trap {
	public:
		int xPos;
		int yPos;
		std::string shape;
		unsigned short color;

	public:
		Trap(int xPos, int yPos, std::string shape, unsigned short color) : xPos(xPos), yPos(yPos), shape(shape), color(color) {}
	};

	class Tunnel {
	public:
		int xPos;
		int yPos;
		std::string shape;
		unsigned short color;
	public:
		Tunnel(int xPos, int yPos, std::string shape, unsigned short color) : xPos(xPos), yPos(yPos), shape(shape), color(color) {}
	};

	class TunnelConnect {
	public:
		Tunnel* startTunnel;
		Tunnel* endTunnel;
	public:
		TunnelConnect(Tunnel* startTunnel, Tunnel* endTunnel) : startTunnel(startTunnel), endTunnel(endTunnel) {};
		void DestroyTunnel() {
			delete startTunnel;
			delete endTunnel;
		}
	};

	//
	unsigned short wallPosX = 1;
	unsigned short wallPosY = 1;


	// Title 코드
	_ESELECT select;

	// InGame 코드
	int stage = 1;
	int score = 0;
	int life = 5;
	std::string userName;

	// 플레이어 스네이크
	std::vector<Snake*> snake;
	std::vector<Snake*>::iterator snakeIter;

	// 아이템
	std::vector<Item*> itemList;
	std::vector<Item*>::iterator itemListIter;

	// 함정
	std::vector<Trap*> trapList;
	std::vector<Trap*>::iterator trapListIter;

	// 터널
	std::vector<TunnelConnect*> tunnelList;
	std::vector<TunnelConnect*>::iterator tunnelListIter;

	// 현재 플레이어 방향
	_EDIRECT direct;

public:
	virtual void OnInit();
	virtual void OnRelease();

	virtual void OnTitleUpdate();
	virtual void OnTitleRender();

	virtual void OnInGameUpdate();
	virtual void OnInGameRender();

	void SnakeInit();
	void ItemInit(int itemCount);
	void TrapInit(int trapCount);
	void TunnelInit(int tunnelCount);

	void MoveSnake();
	void MoveItem(Item* item);
	void MoveTrap(Trap* trap);
};

