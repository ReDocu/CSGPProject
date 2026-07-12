#pragma once
#include "Interface/IGameContent.h"
#include <vector>
#include <string>

// Chrome Dino style endless runner. Follows the CSGP framework (IGameContent phases).
// NOTE: DinoContent.cpp is saved as CP949 (contains block glyphs and Korean UI text).
class DinoContent : public IGameContent
{
private:
	static const int GROUND_Y  = 19;	// ground line row (logical y)
	static const int DINO_X    = 6;		// dino fixed logical x (world scrolls, dino stays)
	static const int DINO_FOOT = 18;	// dino foot y when grounded (GROUND_Y - 1)

	// Internal in-game state (IGameContent::_EPhase stays TITLE / INGAME)
	enum class _EGameState { PLAYING, PAUSE, GAMEOVER };
	enum class _EDinoState { RUN, JUMP, DUCK, DEAD };
	enum class _EObstacle  { CACTUS_SMALL, CACTUS_LARGE, PTERO_LOW, PTERO_MID, PTERO_HIGH };

	struct Obstacle {
		_EObstacle type;
		float x;			// logical x (float, scrolls left)
		int   w, h, topY;	// hitbox size and top row
	};
	struct Cloud {
		float x;
		int   y;
	};

	// dino
	float dinoY;			// foot y (float, for smooth physics)
	float dinoVelY;
	_EDinoState dinoState;
	bool  onGround;

	std::vector<Obstacle> obstacles;
	std::vector<Cloud>    clouds;

	// world
	float gameSpeed;		// cells / frame
	float spawnGap;			// remaining distance until next spawn
	float groundScrollX;	// ground-texture scroll offset

	// score
	float scoreF;			// accumulated (float) -> score
	int   score;
	int   hiScore;			// session best
	int   nightPhase;		// 0 = day, 1 = night
	int   scoreBlink;		// frames left for 100-point blink

	// animation
	int legFrame;			// dino running legs (0/1)
	int wingFrame;			// ptero wings (0/1)

	bool useBeep = true;	// 100-point milestone sound (short, blocking)

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

	void UpdateDino();		// jump / gravity / duck / fast fall
	void UpdateWorld();		// scroll / spawn / despawn / speed
	void UpdateScore();		// score / milestone / day-night
	void SpawnObstacle();
	void SpawnCloud();
	bool CheckCollision();	// AABB dino vs obstacles

	void DrawDino();
	void DrawObstacle(const Obstacle& o);
	void DrawGround();
	void DrawBackground();	// clouds / stars / moon
	void DrawScore();

	int  ForeColor() const;	// foreground color by day/night
	// DrawSpriteRows moved to FrameWork/Render/RenderUtil.h
};
