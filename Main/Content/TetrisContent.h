#pragma once
#include "Interface/IGameContent.h"
#include <vector>
#include <string>

// Tetris game content. Follows the CSGP framework pattern (IGameContent phases).
// NOTE: TetrisContent.cpp is saved as CP949 (contains the block glyph and Korean UI text).
class TetrisContent : public IGameContent
{
private:
	static const int BOARD_W = 10;		// playfield width  (cells)
	static const int BOARD_H = 20;		// playfield height (cells)
	static const int ORIGIN_X = 2;		// logical x of board cell (0,0)
	static const int ORIGIN_Y = 2;		// logical y of board cell (0,0)

	// Internal in-game state (kept out of IGameContent::_EPhase, which is shared)
	enum class _EGameState {
		PLAYING,
		PAUSE,
		GAMEOVER
	};

	// One falling piece. type 0~6 = I,O,T,S,Z,J,L. rot 0~3. (x,y) = board coords.
	struct Tetromino {
		int type;
		int rot;
		int x;
		int y;
	};

	int board[BOARD_H][BOARD_W];	// 0 = empty, 1~7 = locked block (color = type+1)

	Tetromino current;				// active falling piece
	int  holdType;					// held piece type, -1 = none
	bool holdUsed;					// hold allowed once per drop

	std::vector<int> bag;			// 7-bag shuffle buffer
	std::vector<int> nextQueue;		// upcoming pieces (front = next spawn)

	int score;
	int level;
	int lines;

	_EGameState gameState;

	// DAS (Delayed Auto Shift): horizontal auto-repeat move state
	int   dasDir = 0;			// auto-shift direction: -1, 0, +1
	float dasNextMove = 0.0f;	// content-time when the next auto-shift is allowed

public:
	virtual void OnInit();
	virtual void OnRelease();

	virtual void OnTitleUpdate();
	virtual void OnTitleRender();

	virtual void OnInGameUpdate();
	virtual void OnInGameRender();

private:
	void StartGame();				// reset board and start a fresh game

	void RefillBag();				// refill + shuffle the 7-bag
	int  DrawNext();				// pop one piece type from the bag
	void SpawnPiece();				// take next piece; game over if it cannot be placed

	bool CanMove(const Tetromino& p, int dx, int dy, int newRot);
	void Rotate(int dir);			// dir > 0 = CW, dir < 0 = CCW (with wall kick)
	void MoveHorizontal(int dx);
	void HardDrop();
	void LockPiece();				// fix piece to board, clear lines, spawn next
	int  ClearLines();				// remove full rows, return cleared count
	void Hold();
	Tetromino GetGhost();			// landing position of the current piece

	void DrawCell(int col, int row, int color);				// board cell -> console
	void DrawPiece(const Tetromino& p, int color);
	void DrawPreview(int type, int cx, int cy, int color);	// rot0 preview at console (cx,cy)
};
