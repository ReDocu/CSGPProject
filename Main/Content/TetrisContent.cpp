#include "TetrisContent.h"
#include "../../framework.h"

// ============================================================================
// 피스 회전 데이터 (SRS 표준 4상태). 런타임 회전 계산 없이 테이블로 정의한다.
// SHAPES[피스종류][회전상태][블록4개] = { dx, dy }  (4x4 박스 기준)
// 피스종류: 0=I, 1=O, 2=T, 3=S, 4=Z, 5=J, 6=L
// ============================================================================
static const int SHAPES[7][4][4][2] = {
	// I
	{
		{ {0,1},{1,1},{2,1},{3,1} },
		{ {2,0},{2,1},{2,2},{2,3} },
		{ {0,2},{1,2},{2,2},{3,2} },
		{ {1,0},{1,1},{1,2},{1,3} },
	},
	// O
	{
		{ {1,0},{2,0},{1,1},{2,1} },
		{ {1,0},{2,0},{1,1},{2,1} },
		{ {1,0},{2,0},{1,1},{2,1} },
		{ {1,0},{2,0},{1,1},{2,1} },
	},
	// T
	{
		{ {1,0},{0,1},{1,1},{2,1} },
		{ {1,0},{1,1},{2,1},{1,2} },
		{ {0,1},{1,1},{2,1},{1,2} },
		{ {1,0},{0,1},{1,1},{1,2} },
	},
	// S
	{
		{ {1,0},{2,0},{0,1},{1,1} },
		{ {1,0},{1,1},{2,1},{2,2} },
		{ {1,1},{2,1},{0,2},{1,2} },
		{ {0,0},{0,1},{1,1},{1,2} },
	},
	// Z
	{
		{ {0,0},{1,0},{1,1},{2,1} },
		{ {2,0},{1,1},{2,1},{1,2} },
		{ {0,1},{1,1},{1,2},{2,2} },
		{ {1,0},{0,1},{1,1},{0,2} },
	},
	// J
	{
		{ {0,0},{0,1},{1,1},{2,1} },
		{ {1,0},{2,0},{1,1},{1,2} },
		{ {0,1},{1,1},{2,1},{2,2} },
		{ {1,0},{1,1},{0,2},{1,2} },
	},
	// L
	{
		{ {2,0},{0,1},{1,1},{2,1} },
		{ {1,0},{1,1},{1,2},{2,2} },
		{ {0,1},{1,1},{2,1},{0,2} },
		{ {0,0},{1,0},{1,1},{1,2} },
	},
};

// 피스별 색 (framework.h 색 상수). L은 팔레트에 주황이 없어 DARKYELLOW로 대체.
static const int PIECE_COLOR[7] = { SKYBLUE, YELLOW, PURPLE, GREEN, RED, BLUE, DARKYELLOW };

// 회전 시 시도하는 월킥 오프셋(기본형). 순서대로 처음 맞는 위치를 채택한다.
static const int KICKS[8][2] = {
	{0,0}, {-1,0}, {1,0}, {-2,0}, {2,0}, {0,-1}, {-1,-1}, {1,-1}
};

// 레벨별 낙하 속도(초/셀). 레벨이 오를수록 빨라진다.
static const float GRAVITY[15] = {
	0.80f, 0.72f, 0.63f, 0.55f, 0.47f, 0.39f, 0.31f, 0.23f,
	0.17f, 0.13f, 0.10f, 0.08f, 0.07f, 0.06f, 0.05f
};

// ============================================================================
// 생명주기
// ============================================================================
void TetrisContent::OnInit()
{
	IGameContent::OnInit();		// currentPhase = TITLE, TIMER->StartContent()

	currentPhase = _EPhase::TITLE;
	gameState = _EGameState::PLAYING;

	score = 0;
	level = 0;
	lines = 0;
	holdType = -1;
	holdUsed = false;

	for (int r = 0; r < BOARD_H; r++)
		for (int c = 0; c < BOARD_W; c++)
			board[r][c] = 0;

	bag.clear();
	nextQueue.clear();
}

void TetrisContent::OnRelease()
{
	IGameContent::OnRelease();

	bag.clear();
	nextQueue.clear();
}

// 타이틀에서 게임을 새로 시작한다.
void TetrisContent::StartGame()
{
	for (int r = 0; r < BOARD_H; r++)
		for (int c = 0; c < BOARD_W; c++)
			board[r][c] = 0;

	score = 0;
	level = 0;
	lines = 0;
	holdType = -1;
	holdUsed = false;

	bag.clear();
	nextQueue.clear();

	gameState = _EGameState::PLAYING;
	TIMER->StartContent();		// 플레이 타임 기준 리셋

	SpawnPiece();
}

// ============================================================================
// 피스 공급 (7-bag)
// ============================================================================
void TetrisContent::RefillBag()
{
	bag.clear();
	for (int i = 0; i < 7; i++)
		bag.push_back(i);

	// Fisher-Yates 셔플
	for (int i = 6; i > 0; i--)
	{
		int j = rand() % (i + 1);
		int tmp = bag[i];
		bag[i] = bag[j];
		bag[j] = tmp;
	}
}

int TetrisContent::DrawNext()
{
	if (bag.empty())
		RefillBag();

	int t = bag.back();
	bag.pop_back();
	return t;
}

void TetrisContent::SpawnPiece()
{
	// NEXT 미리보기용으로 최소 4개를 채워둔다.
	while ((int)nextQueue.size() < 4)
		nextQueue.push_back(DrawNext());

	int t = nextQueue.front();
	nextQueue.erase(nextQueue.begin());

	current.type = t;
	current.rot = 0;
	current.x = 3;
	current.y = 0;

	// 스폰 위치가 막혀 있으면 게임 오버
	if (!CanMove(current, 0, 0, current.rot))
		gameState = _EGameState::GAMEOVER;
}

// ============================================================================
// 이동 / 회전 / 충돌
// ============================================================================
bool TetrisContent::CanMove(const Tetromino& p, int dx, int dy, int newRot)
{
	for (int i = 0; i < 4; i++)
	{
		int c = p.x + dx + SHAPES[p.type][newRot][i][0];
		int r = p.y + dy + SHAPES[p.type][newRot][i][1];

		if (c < 0 || c >= BOARD_W || r < 0 || r >= BOARD_H)
			return false;
		if (board[r][c] != 0)
			return false;
	}
	return true;
}

void TetrisContent::Rotate(int dir)
{
	int newRot = (current.rot + (dir > 0 ? 1 : 3)) % 4;

	for (int k = 0; k < 8; k++)
	{
		if (CanMove(current, KICKS[k][0], KICKS[k][1], newRot))
		{
			current.x += KICKS[k][0];
			current.y += KICKS[k][1];
			current.rot = newRot;
			return;
		}
	}
	// 모든 킥이 실패하면 회전 취소
}

void TetrisContent::MoveHorizontal(int dx)
{
	if (CanMove(current, dx, 0, current.rot))
		current.x += dx;
}

void TetrisContent::HardDrop()
{
	int dist = 0;
	while (CanMove(current, 0, 1, current.rot))
	{
		current.y++;
		dist++;
	}
	score += dist * 2;		// 하드 드롭 셀당 2점
	LockPiece();
}

void TetrisContent::LockPiece()
{
	for (int i = 0; i < 4; i++)
	{
		int c = current.x + SHAPES[current.type][current.rot][i][0];
		int r = current.y + SHAPES[current.type][current.rot][i][1];
		if (r >= 0 && r < BOARD_H && c >= 0 && c < BOARD_W)
			board[r][c] = current.type + 1;
	}

	int cleared = ClearLines();
	if (cleared > 0)
	{
		static const int lineScore[5] = { 0, 100, 300, 500, 800 };
		score += lineScore[cleared] * (level + 1);
		lines += cleared;
		level = lines / 10;		// 10줄마다 레벨업
	}

	holdUsed = false;
	SpawnPiece();
}

int TetrisContent::ClearLines()
{
	int cleared = 0;

	for (int r = BOARD_H - 1; r >= 0; r--)
	{
		bool full = true;
		for (int c = 0; c < BOARD_W; c++)
		{
			if (board[r][c] == 0)
			{
				full = false;
				break;
			}
		}

		if (full)
		{
			// r 행을 지우고 위쪽을 한 칸씩 내린다.
			for (int rr = r; rr > 0; rr--)
				for (int c = 0; c < BOARD_W; c++)
					board[rr][c] = board[rr - 1][c];
			for (int c = 0; c < BOARD_W; c++)
				board[0][c] = 0;

			cleared++;
			r++;	// 내려온 행을 같은 위치에서 다시 검사
		}
	}
	return cleared;
}

void TetrisContent::Hold()
{
	if (holdUsed)
		return;
	holdUsed = true;

	if (holdType < 0)
	{
		holdType = current.type;
		SpawnPiece();
	}
	else
	{
		int t = holdType;
		holdType = current.type;

		current.type = t;
		current.rot = 0;
		current.x = 3;
		current.y = 0;

		if (!CanMove(current, 0, 0, current.rot))
			gameState = _EGameState::GAMEOVER;
	}
}

TetrisContent::Tetromino TetrisContent::GetGhost()
{
	Tetromino g = current;
	while (CanMove(g, 0, 1, g.rot))
		g.y++;
	return g;
}

// ============================================================================
// 타이틀 페이즈
// ============================================================================
void TetrisContent::OnTitleUpdate()
{
	if (INPUT->OnKeyDown(VK_RETURN))
	{
		StartGame();
		currentPhase = _EPhase::INGAME;
	}
}

void TetrisContent::OnTitleRender()
{
	SCREEN->OnDrawColor(32, 7, "T E T R I S", SKYBLUE);

	SCREEN->OnDraw(24, 11, "PRESS ENTER TO START");

	SCREEN->OnDraw(16, 15, "이동 : 좌우 방향키    회전 : 위 방향키 / Z");
	SCREEN->OnDraw(16, 16, "소프트드롭 : 아래 방향키    하드드롭 : SPACE");
	SCREEN->OnDraw(16, 17, "홀드 : C    일시정지 : P    종료 : ESC");
}

// ============================================================================
// 인게임 페이즈
// ============================================================================
void TetrisContent::OnInGameUpdate()
{
	// ESC -> back to the Tetris title
	if (INPUT->OnKeyDown(VK_ESCAPE))
	{
		currentPhase = _EPhase::TITLE;
		return;
	}

	// 일시정지 토글
	if (INPUT->OnKeyDown('P'))
	{
		if (gameState == _EGameState::PLAYING)
			gameState = _EGameState::PAUSE;
		else if (gameState == _EGameState::PAUSE)
			gameState = _EGameState::PLAYING;
	}

	if (gameState == _EGameState::PAUSE)
		return;

	if (gameState == _EGameState::GAMEOVER)
	{
		// 엔터로 타이틀 복귀
		if (INPUT->OnKeyDown(VK_RETURN))
			currentPhase = _EPhase::TITLE;
		return;
	}

	// ---- PLAYING ----

	// 좌우 이동 (탭 + 자동 반복)
	bool downL = INPUT->OnKeyDown(VK_LEFT);
	bool downR = INPUT->OnKeyDown(VK_RIGHT);
	bool stayL = INPUT->OnKeyStay(VK_LEFT);
	bool stayR = INPUT->OnKeyStay(VK_RIGHT);
	float moveNow = TIMER->GetContentTime();

	if (downL)
	{
		MoveHorizontal(-1);
		dasDir = -1;
		dasNextMove = moveNow + 0.16f;		// DAS initial delay
	}
	else if (downR)
	{
		MoveHorizontal(1);
		dasDir = 1;
		dasNextMove = moveNow + 0.16f;
	}
	else if (dasDir == -1 && stayL)
	{
		if (moveNow >= dasNextMove) { MoveHorizontal(-1); dasNextMove = moveNow + 0.05f; }	// repeat
	}
	else if (dasDir == 1 && stayR)
	{
		if (moveNow >= dasNextMove) { MoveHorizontal(1); dasNextMove = moveNow + 0.05f; }
	}
	else
	{
		dasDir = 0;
	}

	// 회전
	if (INPUT->OnKeyDown(VK_UP) || INPUT->OnKeyDown('X')) Rotate(1);
	if (INPUT->OnKeyDown('Z')) Rotate(-1);

	// 홀드
	if (INPUT->OnKeyDown('C')) Hold();

	// 하드 드롭 (즉시 고정)
	if (INPUT->OnKeyDown(VK_SPACE))
	{
		HardDrop();
		return;
	}

	// 중력 낙하 (아래 방향키 유지 시 소프트 드롭)
	bool soft = INPUT->OnKeyStay(VK_DOWN);
	float speed = soft ? 0.04f : GRAVITY[level < 14 ? level : 14];

	if (TIMER->GetTickTimer(speed))
	{
		if (CanMove(current, 0, 1, current.rot))
		{
			current.y++;
			if (soft) score += 1;		// 소프트 드롭 셀당 1점
		}
		else
		{
			LockPiece();
		}
	}
}

void TetrisContent::OnInGameRender()
{
	// 보드 프레임 (좌/우/하단 벽)
	for (int r = -1; r <= BOARD_H; r++)
	{
		SCREEN->OnDrawColor((ORIGIN_X - 1) * 2, ORIGIN_Y + r, "■", DARKGRAY);
		SCREEN->OnDrawColor((ORIGIN_X + BOARD_W) * 2, ORIGIN_Y + r, "■", DARKGRAY);
	}
	for (int c = -1; c <= BOARD_W; c++)
		SCREEN->OnDrawColor((ORIGIN_X + c) * 2, ORIGIN_Y + BOARD_H, "■", DARKGRAY);

	// 고정 블록
	for (int r = 0; r < BOARD_H; r++)
		for (int c = 0; c < BOARD_W; c++)
			if (board[r][c] != 0)
				DrawCell(c, r, PIECE_COLOR[board[r][c] - 1]);

	// 고스트 + 현재 피스 (게임 오버 시엔 생략)
	if (gameState != _EGameState::GAMEOVER)
	{
		Tetromino g = GetGhost();
		DrawPiece(g, DARKGRAY);
		DrawPiece(current, PIECE_COLOR[current.type]);
	}

	// ---- UI 패널 ----
	int ux = 30;

	SCREEN->OnDrawColor(ux + 3, 1, "T E T R I S", SKYBLUE);

	SCREEN->OnDrawColor(ux, 3, "NEXT", WHITE);
	for (int i = 0; i < 3 && i < (int)nextQueue.size(); i++)
		DrawPreview(nextQueue[i], ux + 2, 5 + i * 3, PIECE_COLOR[nextQueue[i]]);

	SCREEN->OnDrawColor(ux, 14, "HOLD", WHITE);
	if (holdType >= 0)
		DrawPreview(holdType, ux + 2, 16, PIECE_COLOR[holdType]);

	SCREEN->OnDraw(ux, 20, "SCORE");
	SCREEN->OnDraw(ux + 7, 20, std::to_string(score).c_str());
	SCREEN->OnDraw(ux, 21, "LEVEL");
	SCREEN->OnDraw(ux + 7, 21, std::to_string(level).c_str());
	SCREEN->OnDraw(ux, 22, "LINES");
	SCREEN->OnDraw(ux + 7, 22, std::to_string(lines).c_str());
	SCREEN->OnDraw(ux, 23, "TIME");
	SCREEN->OnDraw(ux + 7, 23, std::to_string((int)TIMER->GetContentTime()).c_str());

	// ---- 오버레이 ----
	if (gameState == _EGameState::PAUSE)
		SCREEN->OnDrawColor(7, 11, "-- PAUSE --", YELLOW);

	if (gameState == _EGameState::GAMEOVER)
	{
		SCREEN->OnDrawColor(7, 10, "GAME  OVER", RED);
		SCREEN->OnDrawColor(5, 12, "PRESS ENTER", WHITE);
	}
}

// ============================================================================
// 렌더 헬퍼
// ============================================================================
void TetrisContent::DrawCell(int col, int row, int color)
{
	if (row < 0)
		return;
	SCREEN->OnDrawColor((ORIGIN_X + col) * 2, ORIGIN_Y + row, "■", color);
}

void TetrisContent::DrawPiece(const Tetromino& p, int color)
{
	for (int i = 0; i < 4; i++)
	{
		int c = p.x + SHAPES[p.type][p.rot][i][0];
		int r = p.y + SHAPES[p.type][p.rot][i][1];
		DrawCell(c, r, color);
	}
}

// 회전상태 0 기준으로 콘솔 좌표 (cx, cy)에 미리보기를 그린다.
void TetrisContent::DrawPreview(int type, int cx, int cy, int color)
{
	if (type < 0)
		return;
	for (int i = 0; i < 4; i++)
	{
		int dx = SHAPES[type][0][i][0];
		int dy = SHAPES[type][0][i][1];
		SCREEN->OnDrawColor(cx + dx * 2, cy + dy, "■", color);
	}
}
