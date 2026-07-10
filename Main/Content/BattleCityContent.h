#pragma once
#include "Interface/IGameContent.h"
#include "Pool.h"

// ============================================================================
// Console BATTLE CITY - top-view tank action on the CSGP framework.
//
// Capstone that combines the two curriculum techniques:
//   * Procedural map generation (from MazeContent): each stage builds a
//     battlefield of brick / steel / water / forest / ice + base fortress,
//     then a flood fill verifies every enemy spawn can reach the base.
//   * Object pooling (from GalagaContent): bullets / enemy tanks / explosions
//     / items all live in fixed Pool<T,N> arrays - zero heap churn in-loop.
//
// NOTE: BattleCityContent.cpp is saved as CP949 (full-width glyphs + Korean).
//       This header stays ASCII so it can be edited safely.
// ============================================================================

class BattleCityContent : public IGameContent
{
private:
	// --- battlefield geometry (tile grid) ---
	static const int FIELD_W  = 24;		// tiles across
	static const int FIELD_H  = 20;		// tiles down
	static const int FIELD_X0 = 1;		// logical x of tile (0,0)
	static const int FIELD_Y0 = 2;		// logical y of tile (0,0)

	// --- pool capacities ---
	static const int MAX_PBULLET = 8;
	static const int MAX_EBULLET = 24;
	static const int MAX_ENEMY   = 8;	// pool size
	static const int MAX_FX      = 24;
	static const int MAX_ITEM    = 4;

	static const int SIMUL_MAX   = 4;	// max enemies alive at once
	static const int STAGE_TOTAL = 20;	// enemies spawned per stage

	// Internal in-game state (IGameContent::_EPhase stays TITLE / INGAME)
	enum class _EGameState { STAGESTART, PLAYING, PAUSE, STAGECLEAR, GAMEOVER };
	enum class _ETile      { EMPTY, BRICK, STEEL, WATER, FOREST, ICE, BASE };
	enum class _EDir       { UP, RIGHT, DOWN, LEFT };
	enum class _EEnemyType { BASIC, FAST, ARMOR, ATTACK };
	enum class _EItemType  { STAR, HELMET, CLOCK, SHOVEL, LIFE, BOMB };

	struct Bullet { int x, y; _EDir dir; bool power; bool fromPlayer; };
	struct Tank   { _EEnemyType type; int x, y; _EDir dir; int hp; int dirTimer; int fireTimer; bool flashing; };
	struct Fx     { int x, y; int frame; };
	struct Item   { _EItemType type; int x, y; int blink; };

	_ETile tile[FIELD_H][FIELD_W];

	// --- object pools ---
	Pool<Bullet, MAX_PBULLET> pBullets;
	Pool<Bullet, MAX_EBULLET> eBullets;
	Pool<Tank,   MAX_ENEMY>   enemies;
	Pool<Fx,     MAX_FX>      fxs;
	Pool<Item,   MAX_ITEM>    items;

	// --- player ---
	int   px, py;
	_EDir pdir;
	int   life, powerLevel;
	bool  invincible;
	int   invTimer;
	int   startX, startY;	// respawn tile
	bool  onIce;			// standing on ice this step
	_EDir slideDir;			// pending ice slide
	bool  sliding;

	// --- base / progression ---
	int   baseX, baseY;
	bool  baseAlive;
	int   shieldTimer;		// shovel: fortress is steel while > 0

	int   score, hiScore, stage;
	int   enemiesToSpawn;	// remaining spawn-queue for this stage
	int   freezeTimer;		// clock: enemies frozen while > 0
	int   stageStartTimer;	// STAGESTART intro countdown (frames)
	int   spawnCycle;		// counts spawns -> mark every Nth as flashing
	int   frameCount;		// blink / animation clock

	bool  useBeep = true;

	_EGameState gameState;

public:
	virtual void OnInit();
	virtual void OnRelease();

	virtual void OnTitleUpdate();
	virtual void OnTitleRender();

	virtual void OnInGameUpdate();
	virtual void OnInGameRender();

private:
	void StartGame();			// stage 1 from scratch
	void StartStage(int stage);	// clear pools, generate map, reset stage state

	// --- procedural map generation ---
	void GenerateMap(int stage);
	void PlaceRectSym(_ETile t, int x0, int y0, int w, int h);
	void ClearSpawnZones();
	void CarveConnectivity();	// flood fill: ensure spawns reach player start
	void SetFortress(_ETile t);	// base surrounding tiles

	bool InField(int tx, int ty) const;
	bool Passable(int tx, int ty) const;			// tank can enter
	bool TankAt(int tx, int ty, int skipEnemy) const;	// occupancy (enemies + player)

	void UpdatePlayer();
	void UpdateEnemies();
	void UpdateBullets();
	void UpdateItems();
	void UpdateFx();
	void HandleCollisions();

	void SpawnEnemy();
	bool StepBullet(Bullet& b);		// advance one tile, resolve tile hit; false = die
	void ChooseEnemyDir(Tank& e);	// random or base-biased direction
	void FireBullet(int tx, int ty, _EDir dir, bool power, bool fromPlayer);
	void SpawnFx(int tx, int ty);
	void DropItem(int tx, int ty);
	void ApplyItem(_EItemType type);
	void HitPlayer();
	void KillEnemyAt(int idx);
	int  ScoreOf(_EEnemyType t) const;
	int  ActiveEnemyCount() const;

	void DirDelta(_EDir dir, int& dx, int& dy) const;
	const char* TankGlyph(_EDir dir) const;

	// --- rendering ---
	int  TX(int tx) const { return (FIELD_X0 + tx) * 2; }	// console x
	int  TY(int ty) const { return FIELD_Y0 + ty; }			// console y
	void DrawMap();
	void DrawItemsLayer();
	void DrawTanks();
	void DrawBullets();
	void DrawFx();
	void DrawForestOverlay();	// redraw forest on top -> vision block
	void DrawHUD();
};
