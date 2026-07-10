#pragma once
#include "Interface/IGameContent.h"

// ============================================================================
// Console PAC-MAN - maze action on the CSGP framework (IGameContent phases).
//
// Curriculum stage 8: enemy AI. Four ghosts each compute a DIFFERENT target
// tile (Blinky/Pinky/Inky/Clyde) and pick a direction greedily at junctions,
// driven by a finite state machine (HOUSE/SCATTER/CHASE/FRIGHTENED/EATEN) and
// a global scatter<->chase schedule. See Docs/PacManContent_GDD.md.
//
// NOTE: PacManContent.cpp is saved as CP949 (full-width glyphs + Korean text).
//       This header stays ASCII so it can be edited safely.
// ============================================================================

class PacManContent : public IGameContent
{
private:
	static const int MAZE_W = 19;
	static const int MAZE_H = 21;

	// ghost-house exit tile (just above the gate) and gate location
	static const int EXIT_X = 9, EXIT_Y = 7;

	enum class _EGameState  { READY, PLAYING, PAUSE, DYING, STAGECLEAR, GAMEOVER };
	enum class _ETile       { EMPTY, WALL, PELLET, POWER, DOOR };
	enum class _EDir        { UP, RIGHT, DOWN, LEFT, NONE };
	enum class _EGhost      { BLINKY, PINKY, INKY, CLYDE };
	enum class _EGhostState { HOUSE, SCATTER, CHASE, FRIGHTENED, EATEN };

	struct Ghost {
		_EGhost      who;
		_EGhostState state;
		int   x, y;
		_EDir dir;
		int   spawnX, spawnY;		// house / start tile
		int   scatterX, scatterY;	// scatter corner (aim point)
		int   releaseTimer;			// frames left in the house before leaving
	};

	_ETile tile[MAZE_H][MAZE_W];
	int    fx0, fy0;				// field offset for centering
	int    pelletCount;

	// player
	int   px, py, startX, startY;
	_EDir pdir, wantDir;
	int   life, score, hiScore, stage;
	int   powerTimer;				// > 0 while power mode active
	int   eatChain;					// frightened-ghost chain index (200<<n)

	// ghosts / global mode
	Ghost ghosts[4];
	int   modeTimer;				// scatter/chase schedule countdown
	bool  chaseMode;

	int   readyTimer, dyingTimer;
	int   frameCount;
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
	void StartGame();
	void StartStage(int stage);
	void LoadMaze();				// authored char rows -> tile[][], find start
	void ResetActors();				// place pac + ghosts at spawns

	void UpdatePlayer();
	void UpdateGhosts();			// mode schedule + per-ghost move
	void ChooseGhostDir(Ghost& g);	// personality target + greedy pick
	void GhostTarget(const Ghost& g, int& tx, int& ty);
	void HandleCollisions();
	void KillPlayer();
	void SetFrightened();			// power pellet -> all ghosts flee

	int   PowerFrames() const;		// per-stage power duration

	bool  Passable(int tx, int ty, bool isGhost) const;
	void  DirDelta(_EDir d, int& dx, int& dy) const;
	_EDir Reverse(_EDir d) const;
	int   WrapX(int tx) const;

	int   CX(int tx) const { return (fx0 + tx) * 2; }	// console x
	int   CY(int ty) const { return fy0 + ty; }			// console y
	const char* PacGlyph() const;

	void DrawMaze();
	void DrawGhosts();
	void DrawPlayer();
	void DrawHUD();
};
