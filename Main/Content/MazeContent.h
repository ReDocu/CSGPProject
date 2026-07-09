#pragma once
#include "Interface/IGameContent.h"
#include <vector>
#include <string>

// Algorithm maze. Follows the CSGP framework (IGameContent phases).
// Each stage builds its maze with a DIFFERENT generation algorithm, so the
// maze's character changes per stage. Pathfinding (BFS/DFS/A*/wall-follower)
// is a shared learning mechanic (HINT / SOLVE visualization).
//
// Player, pathfinding and scoring all work in GRID coordinates (one grid cell
// = one glyph step), so the player walks room -> corridor -> room one cell at a
// time. Generation still uses the (2*cell+1) wall-grid model.
//
// NOTE: MazeContent.cpp is saved as CP949 (contains block glyphs).
class MazeContent : public IGameContent
{
private:
	static const int CELL_COLS = 13;			// rooms per row
	static const int CELL_ROWS = 10;			// rooms per column
	static const int GRID_W    = CELL_COLS * 2 + 1;	// 27 (cells + walls between)
	static const int GRID_H    = CELL_ROWS * 2 + 1;	// 21
	static const int NUM_CELLS = CELL_COLS * CELL_ROWS;	// 130 (generation)
	static const int NUM_GRID  = GRID_W * GRID_H;		// 567 (movement / pathfinding)
	static const int MAZE_X0   = 1;				// logical x of grid (0,0)
	static const int MAZE_Y0   = 1;				// logical y of grid (0,0)
	static const int HUD_X     = 58;			// console column of the HUD panel

	enum class _EGameState { PLAYING, SOLVING, CLEAR, PAUSE };
	enum class _EGenAlgo   { BINARY_TREE, BACKTRACKER, PRIM, KRUSKAL, DIVISION, GEN_COUNT };
	enum class _ESolveAlgo { BFS, DFS, ASTAR, WALL_FOLLOWER, SOLVE_COUNT };

	bool wall[GRID_H][GRID_W];	// true = wall, false = passage

	int  pgx, pgy;				// player  (grid coords, 1 cell per step)
	int  sgx, sgy;				// start   (grid coords)
	int  egx, egy;				// exit    (grid coords)

	int  stage;					// 0..4 (== _EGenAlgo index)
	_EGenAlgo   genAlgo;
	_ESolveAlgo solveAlgo;
	_EGameState gameState;

	int  steps;					// player grid-steps this stage
	int  optimalSteps;			// BFS shortest (grid) for scoring
	int  stageScore, totalScore, hiScore;
	int  hintsUsed;
	int  stars;
	bool usedSolve;				// used SOLVE this stage -> no efficiency bonus
	unsigned int seed;			// current maze seed (shown / R re-rolls)

	std::vector<int> hintCells;	// next few shortest-path grid cells (H)
	int  hintBlink;				// frames left for hint blink

	std::vector<int> solveOrder;	// search expansion order (F visualization)
	std::vector<int> solvePath;		// final route
	int  solveAnimIdx;				// reveal cursor for the animation

	float moveNextTime;				// content-time gate for held-move auto-repeat (DAS)

public:
	virtual void OnInit();
	virtual void OnRelease();

	virtual void OnTitleUpdate();
	virtual void OnTitleRender();

	virtual void OnInGameUpdate();
	virtual void OnInGameRender();

private:
	void StartStage(int stageIndex);

	// generation (stage axis) -- works in cell space, writes the wall grid
	void GenerateMaze(_EGenAlgo algo);
	void ResetWallsAll(bool val);
	void OpenAllCells();
	void GenBinaryTree();
	void GenBacktracker();
	void GenPrim();
	void GenKruskal();
	void GenDivision(int gx0, int gy0, int gx1, int gy1);

	// pathfinding / movement (grid space)
	bool GridOpen(int gx, int gy, int dir) const;			// is the adjacent grid cell passable
	int  ShortestPath(int startCell, int goalCell, std::vector<int>& pathOut) const;
	void BuildSolve(_ESolveAlgo algo);						// fills solveOrder / solvePath
	void BuildHint();

	void MovePlayer(int dir);
	void FinishStage();

	// render
	void DrawMaze();
	void DrawStartExit();
	void DrawPlayer();
	void DrawSolveOverlay();
	void DrawHintOverlay();
	void DrawClearOverlay();
	void DrawHUD();
	void DrawGridGlyph(int gx, int gy, const char* glyph, int color);

	int  WallColor() const;
	const char* GenAlgoName(_EGenAlgo a) const;
	const char* SolveAlgoName(_ESolveAlgo a) const;
};
