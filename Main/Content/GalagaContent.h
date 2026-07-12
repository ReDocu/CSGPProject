#pragma once
#include "Interface/IGameContent.h"
#include "../../FrameWork/Object/Pool.h"

// ============================================================================
// Console GALAGA - shooting arcade on the CSGP framework (IGameContent phases).
//
// Curriculum theme: OBJECT POOL. Bullets / enemies / explosions are all short-
// lived objects created and destroyed dozens of times per second. Instead of
// per-frame new/delete or vector rebuild (see Dino / RoadFighter), every such
// object lives in a fixed-size Pool<T,N> and is toggled with an 'active' flag.
// Inside the game loop there is ZERO heap allocation / deallocation.
//
// NOTE: GalagaContent.cpp is saved as CP949 (full-width glyphs + Korean UI text).
//       This header stays ASCII so it can be edited safely.
// ============================================================================

class GalagaContent : public IGameContent
{
private:
	// --- layout (logical coordinates; render multiplies x by 2) ---
	static const int HUD_Y       = 1;	// top info row
	static const int FORM_TOP    = 3;	// formation top row
	static const int PLAY_LEFT   = 1;	// left movement / wall limit
	static const int PLAY_RIGHT  = 37;	// right movement / wall limit
	static const int PLAYER_Y    = 21;	// player fixed y (x moves only)
	static const int DIVIDER_Y   = 22;	// divider line
	static const int CONTROL_Y   = 23;	// control hint row
	static const int SPACING_X   = 4;	// horizontal gap between formation columns
	static const int SPACING_Y   = 2;	// vertical gap between formation rows

	// --- pool capacities ---
	static const int MAX_PBULLET = 32;
	static const int MAX_EBULLET = 64;
	static const int MAX_ENEMY   = 40;
	static const int MAX_FX      = 32;

	// Internal in-game state (IGameContent::_EPhase stays TITLE / INGAME)
	enum class _EGameState { PLAYING, PAUSE, STAGECLEAR, GAMEOVER };
	enum class _EEnemyType { BASIC, FAST, STRONG };

	struct Bullet { float x, y, vx, vy; };
	struct Enemy  { _EEnemyType type; int col, row, hp; };
	struct Fx     { float x, y; int frame; };

	// --- object pools (spec: BulletManager / EnemyManager / FX) ---
	Pool<Bullet, MAX_PBULLET> pBullets;	// player bullets (up)
	Pool<Bullet, MAX_EBULLET> eBullets;	// enemy bullets (down)
	Pool<Enemy,  MAX_ENEMY>   enemies;
	Pool<Fx,     MAX_FX>      fxs;

	// --- player ---
	float px;			// logical x (float, clamped to PLAY_LEFT..PLAY_RIGHT)
	int   life;
	int   score;
	int   hiScore;		// session best
	bool  invincible;	// post-hit invincibility
	int   invTimer;		// invincibility frames left

	// --- formation ---
	float formX;		// x of column 0
	int   formY;		// rows descended (added to FORM_TOP)
	int   formDir;		// march direction (+1 / -1)
	int   formCols, formRows;
	int   stage;
	int   aliveCount;	// remaining enemies this stage

	int   frameCount;	// blink / starfield animation
	bool  useBeep = true;	// short sound cues (blocking)

	_EGameState gameState;

public:
	virtual void OnInit();
	virtual void OnRelease();

	virtual void OnTitleUpdate();
	virtual void OnTitleRender();

	virtual void OnInGameUpdate();
	virtual void OnInGameRender();

private:
	void StartGame();			// stage 1 from scratch, clear every pool
	void SpawnStage(int stage);	// fill enemy pool with a formation

	void UpdatePlayer();		// move / fire / invincibility
	void UpdateEnemies();		// formation march + attack
	void UpdateBullets();		// pBullets + eBullets travel / despawn
	void UpdateFx();			// explosion animation
	void HandleCollisions();	// pool x pool

	void HitPlayer();			// life-- / invincibility / fx
	void SpawnFx(float x, float y);
	bool EnemiesBreached() const;	// formation reached the player row

	float FormTick() const;		// formation step interval (by stage)
	float AttackTick() const;	// enemy fire interval (by stage)
	int   ScoreOf(_EEnemyType t) const;

	int   EnemyX(const Enemy& e) const;	// formX + col * SPACING_X
	int   EnemyY(const Enemy& e) const;	// FORM_TOP + formY + row * SPACING_Y

	void DrawStars();
	void DrawEnemies();
	void DrawBullets();
	void DrawFx();
	void DrawPlayer();
	void DrawHUD();
};
