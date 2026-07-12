#pragma once
#include "Interface/IGameContent.h"
#include <vector>
#include <string>

// Road Fighter style console racer. Follows the CSGP framework (IGameContent phases).
// NOTE: RoadFighterContent.cpp is saved as CP949 (block/item glyphs + Korean UI text).
class RoadFighterContent : public IGameContent
{
private:
	static const int ROAD_TOP    = 4;	// road top row (below HUD)
	static const int ROAD_BOTTOM = 22;	// road bottom boundary row
	static const int ROAD_LEFT   = 1;	// left wall x
	static const int ROAD_RIGHT  = 38;	// right wall x
	static const int LANE_COUNT  = 5;
	static const int PLAYER_Y    = 20;	// player foot (bottom) row

	// Internal in-game state (IGameContent::_EPhase stays TITLE / INGAME)
	enum class _EGameState { PLAYING, PAUSE, GAMEOVER };
	enum class _EEntity    { ENEMY_NORMAL, ENEMY_FAST, TRUCK, FUEL, BONUS, OBSTACLE };

	struct Entity {
		_EEntity type;
		int   lane;		// lane index (0..LANE_COUNT-1)
		float y;		// top row (float, scrolls down)
		int   w, h;		// hitbox size in cells
	};

	// player
	int   playerLane;
	float speed;		// km/h display value
	float fuelF;		// 0..100
	int   life;
	bool  invincible;	// post-hit invincibility
	int   invTimer;		// invincibility frames left

	std::vector<Entity> entities;

	// world
	float scrollAccum;	// accumulates cells scrolled (for distance/score)
	float roadScroll;	// lane-divider scroll offset

	int   score;
	int   hiScore;
	int   distance;		// meters
	int   stage;

	bool  useBeep = true;	// item pickup sound (short, blocking)

	_EGameState gameState;

public:
	virtual void OnInit();
	virtual void OnRelease();

	virtual void OnTitleUpdate();
	virtual void OnTitleRender();

	virtual void OnInGameUpdate();
	virtual void OnInGameRender();

private:
	void StartGame();

	void UpdatePlayer();		// lane move / accelerate / invincibility
	void UpdateWorld();			// scroll / spawn / despawn / dodge score
	void UpdateFuel();
	void UpdateStage();
	void SpawnEntity();
	void HandleCollisions();	// items collect, vehicles damage

	int  LaneCenterX(int lane) const;

	void DrawRoad();
	void DrawEntities();
	void DrawPlayer();
	void DrawHUD();

	void DrawBlock(int left, int top, int w, int h, int color);				// solid vehicle
	// DrawSpriteRows moved to FrameWork/Render/RenderUtil.h
};
