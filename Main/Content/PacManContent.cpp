#include "PacManContent.h"
#include "../../framework.h"
#include "../../FrameWork/UI/TextUtil.h"
using TextUtil::Pad;

// ============================================================================
// 튜닝 상수 (실제 플레이하며 조정). GetTickTimer 키는 서로 다른 값이어야 한다.
// ============================================================================
static const float PAC_MOVE     = 0.13f;	// 팩맨 이동 간격(초)
static const float GHOST_MOVE   = 0.15f;	// 고스트 일반 이동(팩맨보다 약간 느림)
static const float GHOST_FRIGHT = 0.26f;	// 프라이트(느림, 잡기 쉬움)
static const float GHOST_EATEN  = 0.06f;	// 먹힌 눈(빠르게 귀환)
static const float GHOST_HOUSE  = 0.28f;	// 하우스 탈출

static const int SCATTER_FRAMES = 420;		// 약 7초
static const int CHASE_FRAMES   = 1200;		// 약 20초
static const int READY_FRAMES   = 90;
static const int DYING_FRAMES   = 72;

// 검증된 미로 (Python flood-fill 로 전 펠릿 도달 확인). 19 x 21.
//  # 벽 · . 펠릿 · o 파워 · - 게이트 · P 팩맨시작 · 공백 통로/하우스/터널
static const char* MAZE[21] = {
	"###################",
	"#o...............o#",
	"#.##.##.##.##.##.##",
	"#.##.##.##.##.##.##",
	"#.................#",
	"#.##.##.##.##.##.##",
	"#.##.##.##.##.##.##",
	"#.................#",
	"#.##.####-###.##.##",
	"#.##.###   ##.##.##",
	" ......#   #...... ",
	"#.##.########.##.##",
	"#.##.##.##.##.##.##",
	"#.................#",
	"#.##.##.##.##.##.##",
	"#.##.##.##.##.##.##",
	"#........P........#",
	"#.##.##.##.##.##.##",
	"#.##.##.##.##.##.##",
	"#o...............o#",
	"###################",
};

// Pad(): moved to FrameWork/UI/TextUtil.h

// ============================================================================
// 생명주기
// ============================================================================
void PacManContent::OnInit()
{
	IGameContent::OnInit();		// currentPhase = TITLE, TIMER->StartContent()
	currentPhase = _EPhase::TITLE;

	hiScore = 0;

	fx0 = (40 - MAZE_W) / 2;		// 중앙 정렬
	fy0 = 2;

	score = 0;
	life = 3;
	stage = 1;

	LoadMaze();
	ResetActors();

	frameCount = 0;
	gameState = _EGameState::READY;
	readyTimer = READY_FRAMES;
}

void PacManContent::OnRelease()
{
	IGameContent::OnRelease();
}

void PacManContent::StartGame()
{
	score = 0;
	life = 3;
	stage = 1;
	StartStage(stage);
}

void PacManContent::StartStage(int s)
{
	stage = s;
	LoadMaze();
	ResetActors();
	frameCount = 0;
	gameState = _EGameState::READY;
	readyTimer = READY_FRAMES;
	TIMER->StartContent();
}

// 문자 배열 -> 타일 격자 + 시작 지점 탐색
void PacManContent::LoadMaze()
{
	pelletCount = 0;
	startX = MAZE_W / 2;
	startY = MAZE_H - 5;

	for (int r = 0; r < MAZE_H; r++)
	{
		for (int c = 0; c < MAZE_W; c++)
		{
			char ch = MAZE[r][c];
			switch (ch)
			{
			case '#': tile[r][c] = _ETile::WALL;  break;
			case '.': tile[r][c] = _ETile::PELLET; pelletCount++; break;
			case 'o': tile[r][c] = _ETile::POWER;  pelletCount++; break;
			case '-': tile[r][c] = _ETile::DOOR;  break;
			case 'P': tile[r][c] = _ETile::EMPTY; startX = c; startY = r; break;
			default:  tile[r][c] = _ETile::EMPTY; break;
			}
		}
	}
}

// 팩맨 + 고스트를 스폰 위치/상태로 배치
void PacManContent::ResetActors()
{
	px = startX; py = startY;
	pdir = _EDir::LEFT; wantDir = _EDir::NONE;

	powerTimer = 0;
	eatChain = 0;
	modeTimer = SCATTER_FRAMES;
	chaseMode = false;

	// all four ghosts start OUTSIDE the house, active from the start
	ghosts[0] = { _EGhost::BLINKY, _EGhostState::SCATTER, 9, 7, _EDir::RIGHT, 9, 7, MAZE_W - 2, 1, 0 };
	ghosts[1] = { _EGhost::PINKY, _EGhostState::SCATTER, 8, 7, _EDir::LEFT, 8, 7, 1, 1, 0 };
	ghosts[2] = { _EGhost::INKY, _EGhostState::SCATTER, 10, 7, _EDir::RIGHT, 10, 7, MAZE_W - 2, MAZE_H - 2, 0 };
	ghosts[3] = { _EGhost::CLYDE, _EGhostState::SCATTER, 7, 7, _EDir::LEFT, 7, 7, 1, MAZE_H - 2, 0 };
}

// ============================================================================
// 타이틀
// ============================================================================
static int s_titleSelect = 0;	// 0: 게임 시작, 1: 게임 종료

void PacManContent::OnTitleUpdate()
{
	if (INPUT->OnKeyDown(VK_UP) || INPUT->OnKeyDown(VK_DOWN))
		s_titleSelect = 1 - s_titleSelect;

	if (INPUT->OnKeyDown(VK_RETURN))
	{
		if (s_titleSelect == 0)
		{
			StartGame();
			currentPhase = _EPhase::INGAME;
		}
		else
		{
			s_titleSelect = 0;
			SCENE->ChangeContentWithLoading((int)_ECONTENT::LOAD, (int)_ECONTENT::TITLE);
		}
	}
}

void PacManContent::OnTitleRender()
{
	SCREEN->OnDrawColor(5, 3, "■■■■■      ■      ■■■■■  ■      ■      ■      ■      ■", YELLOW);
	SCREEN->OnDrawColor(5, 4, "■      ■    ■  ■    ■          ■■  ■■    ■  ■    ■    ■■", YELLOW);
	SCREEN->OnDrawColor(5, 5, "■■■■■  ■■■■■  ■          ■  ■  ■  ■■■■■  ■  ■  ■", YELLOW);
	SCREEN->OnDrawColor(5, 6, "■          ■      ■  ■          ■      ■  ■      ■  ■■    ■", YELLOW);
	SCREEN->OnDrawColor(5, 7, "■          ■      ■  ■■■■■  ■      ■  ■      ■  ■      ■", YELLOW);

	SCREEN->OnDrawColor(24 * 2, 10, "▶", YELLOW);
	SCREEN->OnDrawColor(27 * 2, 10, "Ω", RED);
	SCREEN->OnDrawColor(29 * 2, 10, "Ω", SKYBLUE);
	SCREEN->OnDrawColor(31 * 2, 10, "Ω", PURPLE);
	SCREEN->OnDrawColor(33 * 2, 10, "Ω", DARKYELLOW);

	if (hiScore > 0)
		SCREEN->OnDrawColor(30, 11, ("HI  " + Pad(hiScore, 6)).c_str(), GRAY);

	SCREEN->OnDrawColor(25, s_titleSelect == 0 ? 13 : 16, "▶", RED);
	SCREEN->OnDrawColor(32, 13, "게 임 시 작", BLUE);
	SCREEN->OnDrawColor(32, 16, "게 임 종 료", BLUE);

	SCREEN->OnDrawColor(16, 19, "WASD: 이동   P: 일시정지   ESC: 타이틀로", GRAY);
	SCREEN->OnDrawColor(8, 20, "펠릿을 모두 먹으면 클리어 - 파워펠릿(●)을 먹으면 고스트를 반격", GRAY);
}

// ============================================================================
// 인게임
// ============================================================================
void PacManContent::OnInGameUpdate()
{
	if (INPUT->OnKeyDown(VK_ESCAPE))
	{
		currentPhase = _EPhase::TITLE;
		return;
	}

	if (INPUT->OnKeyDown('P'))
	{
		if (gameState == _EGameState::PLAYING)    gameState = _EGameState::PAUSE;
		else if (gameState == _EGameState::PAUSE) gameState = _EGameState::PLAYING;
	}

	switch (gameState)
	{
	case _EGameState::READY:
		readyTimer--;
		if (readyTimer <= 0) gameState = _EGameState::PLAYING;
		return;

	case _EGameState::PAUSE:
		return;

	case _EGameState::DYING:
		dyingTimer--;
		if (dyingTimer <= 0)
		{
			if (life <= 0)
			{
				if (score > hiScore) hiScore = score;
				gameState = _EGameState::GAMEOVER;
			}
			else
			{
				ResetActors();
				gameState = _EGameState::READY;
				readyTimer = READY_FRAMES;
			}
		}
		return;

	case _EGameState::STAGECLEAR:
		if (INPUT->OnKeyDown(VK_SPACE) || INPUT->OnKeyDown(VK_RETURN))
			StartStage(stage + 1);
		return;

	case _EGameState::GAMEOVER:
		if (INPUT->OnKeyDown(VK_SPACE) || INPUT->OnKeyDown(VK_RETURN))
			StartGame();
		return;

	default: break;	// PLAYING
	}

	// ---- PLAYING ----
	frameCount++;

	// 방향 예약(버퍼링): 매 프레임 갱신, 눌린 게 없으면 직전 방향 유지
	if (INPUT->OnKeyStay('W') || INPUT->OnKeyStay(VK_UP))         wantDir = _EDir::UP;
	else if (INPUT->OnKeyStay('S') || INPUT->OnKeyStay(VK_DOWN))  wantDir = _EDir::DOWN;
	else if (INPUT->OnKeyStay('A') || INPUT->OnKeyStay(VK_LEFT))  wantDir = _EDir::LEFT;
	else if (INPUT->OnKeyStay('D') || INPUT->OnKeyStay(VK_RIGHT)) wantDir = _EDir::RIGHT;

	UpdatePlayer();
	UpdateGhosts();
	HandleCollisions();

	if (gameState == _EGameState::PLAYING && pelletCount <= 0)
	{
		score += 1000;
		if (score > hiScore) hiScore = score;
		gameState = _EGameState::STAGECLEAR;
		if (useBeep) Beep(1800, 40);
	}
}

// ============================================================================
// 팩맨 (이동 버퍼링 + 터널 + 펠릿)
// ============================================================================
void PacManContent::UpdatePlayer()
{
	if (!TIMER->GetTickTimer(PAC_MOVE))
		return;

	// 예약 방향이 통행 가능하면 회전
	if (wantDir != _EDir::NONE)
	{
		int dx, dy; DirDelta(wantDir, dx, dy);
		if (Passable(WrapX(px + dx), py + dy, false))
			pdir = wantDir;
	}

	// 진행 방향으로 1칸
	int dx, dy; DirDelta(pdir, dx, dy);
	int nx = WrapX(px + dx), ny = py + dy;
	if (Passable(nx, ny, false))
	{
		px = nx; py = ny;
	}

	// 획득
	if (tile[py][px] == _ETile::PELLET)
	{
		tile[py][px] = _ETile::EMPTY;
		score += 10; pelletCount--;
	}
	else if (tile[py][px] == _ETile::POWER)
	{
		tile[py][px] = _ETile::EMPTY;
		score += 50; pelletCount--;
		SetFrightened();
		if (useBeep) Beep(700, 12);
	}
}

// ============================================================================
// 고스트 (전역 모드 스케줄 + 성격별 AI/FSM)
// ============================================================================
void PacManContent::UpdateGhosts()
{
	// 전역 scatter <-> chase 스케줄 (파워모드 중엔 정지)
	if (powerTimer <= 0)
	{
		modeTimer--;
		if (modeTimer <= 0)
		{
			chaseMode = !chaseMode;
			modeTimer = chaseMode ? CHASE_FRAMES : SCATTER_FRAMES;
			for (int i = 0; i < 4; i++)
				if (ghosts[i].state == _EGhostState::SCATTER || ghosts[i].state == _EGhostState::CHASE)
				{
					ghosts[i].state = chaseMode ? _EGhostState::CHASE : _EGhostState::SCATTER;
					ghosts[i].dir = Reverse(ghosts[i].dir);	// 모드 전환 시 반전
				}
		}
	}
	else
	{
		powerTimer--;
		if (powerTimer == 0)
		{
			for (int i = 0; i < 4; i++)
				if (ghosts[i].state == _EGhostState::FRIGHTENED)
					ghosts[i].state = chaseMode ? _EGhostState::CHASE : _EGhostState::SCATTER;
			eatChain = 0;
		}
	}

	bool tN = TIMER->GetTickTimer(GHOST_MOVE);
	bool tF = TIMER->GetTickTimer(GHOST_FRIGHT);
	bool tE = TIMER->GetTickTimer(GHOST_EATEN);
	bool tH = TIMER->GetTickTimer(GHOST_HOUSE);

	for (int i = 0; i < 4; i++)
	{
		Ghost& g = ghosts[i];

		bool moveNow =
			(g.state == _EGhostState::FRIGHTENED) ? tF :
			(g.state == _EGhostState::EATEN)      ? tE :
			(g.state == _EGhostState::HOUSE)      ? tH : tN;
		if (!moveNow) continue;

		if (g.state == _EGhostState::HOUSE)
		{
			if (g.releaseTimer > 0) { g.releaseTimer--; continue; }	// 대기
			// 방출: 게이트 위 EXIT 로 향한다 (ChooseGhostDir 이 target 처리)
		}

		ChooseGhostDir(g);
		int dx, dy; DirDelta(g.dir, dx, dy);
		int gnx = WrapX(g.x + dx), gny = g.y + dy;
		if (Passable(gnx, gny, true)) { g.x = gnx; g.y = gny; }

		// 하우스 탈출 완료 -> 전역 모드
		if (g.state == _EGhostState::HOUSE && g.y <= EXIT_Y)
			g.state = chaseMode ? _EGhostState::CHASE : _EGhostState::SCATTER;

		// 먹힌 눈이 하우스 입구 도달 -> 부활
		if (g.state == _EGhostState::EATEN && g.x == EXIT_X && g.y == EXIT_Y)
			g.state = chaseMode ? _EGhostState::CHASE : _EGhostState::SCATTER;
	}
}

// 성격별 목표 타일로 greedy 선택 (역방향 금지, 프라이트는 랜덤)
void PacManContent::ChooseGhostDir(Ghost& g)
{
	int tgtX, tgtY; GhostTarget(g, tgtX, tgtY);
	_EDir rev = Reverse(g.dir);

	_EDir cand[4];
	int   n = 0;
	// tie-break 우선순위: 위 > 왼 > 아래 > 오른
	_EDir order[4] = { _EDir::UP, _EDir::LEFT, _EDir::DOWN, _EDir::RIGHT };
	for (int k = 0; k < 4; k++)
	{
		_EDir d = order[k];
		if (d == rev) continue;
		int dx, dy; DirDelta(d, dx, dy);
		int nx = WrapX(g.x + dx), ny = g.y + dy;
		if (Passable(nx, ny, true))
			cand[n++] = d;
	}

	if (n == 0)
	{
		g.dir = rev;	// 막다른 길: 역방향 허용
		return;
	}

	if (g.state == _EGhostState::FRIGHTENED)
	{
		g.dir = cand[rand() % n];	// 프라이트: 랜덤
		return;
	}

	// 목표까지 유클리드 제곱 최소
	_EDir best = cand[0];
	long bestD = 1L << 29;
	for (int k = 0; k < n; k++)
	{
		int dx, dy; DirDelta(cand[k], dx, dy);
		int nx = g.x + dx, ny = g.y + dy;
		long dd = (long)(nx - tgtX) * (nx - tgtX) + (long)(ny - tgtY) * (ny - tgtY);
		if (dd < bestD) { bestD = dd; best = cand[k]; }
	}
	g.dir = best;
}

void PacManContent::GhostTarget(const Ghost& g, int& tx, int& ty)
{
	if (g.state == _EGhostState::EATEN || g.state == _EGhostState::HOUSE)
	{
		tx = EXIT_X; ty = EXIT_Y; return;
	}
	if (g.state == _EGhostState::SCATTER)
	{
		tx = g.scatterX; ty = g.scatterY; return;
	}
	// CHASE (FRIGHTENED 은 랜덤이라 target 미사용)
	int dx, dy; DirDelta(pdir, dx, dy);
	switch (g.who)
	{
	case _EGhost::BLINKY:
		tx = px; ty = py;
		break;
	case _EGhost::PINKY:
		tx = px + 4 * dx; ty = py + 4 * dy;	// 4칸 앞
		break;
	case _EGhost::INKY:
	{
		int ax = px + 2 * dx, ay = py + 2 * dy;			// 팩맨 2칸 앞
		int bx = ghosts[0].x, by = ghosts[0].y;			// Blinky
		tx = ax + (ax - bx); ty = ay + (ay - by);		// 반사 벡터(협공)
		break;
	}
	default: // CLYDE
	{
		int man = abs(g.x - px) + abs(g.y - py);
		if (man > 8) { tx = px; ty = py; }
		else         { tx = g.scatterX; ty = g.scatterY; }
		break;
	}
	}
}

void PacManContent::SetFrightened()
{
	powerTimer = PowerFrames();
	eatChain = 0;
	for (int i = 0; i < 4; i++)
	{
		if (ghosts[i].state == _EGhostState::SCATTER || ghosts[i].state == _EGhostState::CHASE)
		{
			ghosts[i].state = _EGhostState::FRIGHTENED;
			ghosts[i].dir = Reverse(ghosts[i].dir);
		}
	}
}

int PacManContent::PowerFrames() const
{
	int f = 480 - (stage - 1) * 60;	// 스테이지 오를수록 짧아짐
	return (f < 120) ? 120 : f;
}

// ============================================================================
// 충돌
// ============================================================================
void PacManContent::HandleCollisions()
{
	for (int i = 0; i < 4; i++)
	{
		Ghost& g = ghosts[i];
		if (g.x != px || g.y != py) continue;

		if (g.state == _EGhostState::FRIGHTENED)
		{
			int pts = 200 << (eatChain < 3 ? eatChain : 3);	// 200/400/800/1600
			score += pts;
			eatChain++;
			g.state = _EGhostState::EATEN;
			if (useBeep) Beep(1000, 10);
		}
		else if (g.state == _EGhostState::SCATTER || g.state == _EGhostState::CHASE)
		{
			KillPlayer();
			return;
		}
		// EATEN / HOUSE 는 무시
	}
}

void PacManContent::KillPlayer()
{
	life--;
	if (useBeep) Beep(200, 150);
	gameState = _EGameState::DYING;
	dyingTimer = DYING_FRAMES;
}

// ============================================================================
// 헬퍼
// ============================================================================
bool PacManContent::Passable(int tx, int ty, bool isGhost) const
{
	if (ty < 0 || ty >= MAZE_H || tx < 0 || tx >= MAZE_W) return false;
	_ETile t = tile[ty][tx];
	if (t == _ETile::WALL) return false;
	if (t == _ETile::DOOR) return isGhost;
	return true;
}

void PacManContent::DirDelta(_EDir d, int& dx, int& dy) const
{
	switch (d)
	{
	case _EDir::UP:    dx = 0;  dy = -1; break;
	case _EDir::RIGHT: dx = 1;  dy = 0;  break;
	case _EDir::DOWN:  dx = 0;  dy = 1;  break;
	case _EDir::LEFT:  dx = -1; dy = 0;  break;
	default:           dx = 0;  dy = 0;  break;
	}
}

PacManContent::_EDir PacManContent::Reverse(_EDir d) const
{
	switch (d)
	{
	case _EDir::UP:    return _EDir::DOWN;
	case _EDir::DOWN:  return _EDir::UP;
	case _EDir::LEFT:  return _EDir::RIGHT;
	case _EDir::RIGHT: return _EDir::LEFT;
	default:           return _EDir::NONE;
	}
}

int PacManContent::WrapX(int tx) const
{
	return (tx + MAZE_W) % MAZE_W;
}

const char* PacManContent::PacGlyph() const
{
	// 입 여닫기 애니
	if ((frameCount / 6) % 2 == 0)
		return "●";
	switch (pdir)
	{
	case _EDir::UP:    return "▲";
	case _EDir::DOWN:  return "▼";
	case _EDir::LEFT:  return "◀";
	default:           return "▶";
	}
}

// ============================================================================
// 렌더링
// ============================================================================
void PacManContent::OnInGameRender()
{
	DrawMaze();
	DrawGhosts();
	DrawPlayer();
	DrawHUD();

	SCREEN->OnDraw(2, 23, "WASD 이동   P 정지   ESC 뒤로");

	if (gameState == _EGameState::READY)
		SCREEN->OnDrawColor(CX(6), CY(12), "R E A D Y !", YELLOW);

	if (gameState == _EGameState::PAUSE)
		SCREEN->OnDrawColor(CX(6), CY(10), "-- PAUSE --", YELLOW);

	if (gameState == _EGameState::STAGECLEAR)
	{
		SCREEN->OnDrawColor(CX(4), CY(9),  ("STAGE " + std::to_string(stage) + " CLEAR").c_str(), YELLOW);
		SCREEN->OnDrawColor(CX(5), CY(11), "BONUS +1000", GREEN);
		SCREEN->OnDraw(24, CY(13), "PRESS ENTER");
	}

	if (gameState == _EGameState::GAMEOVER)
	{
		SCREEN->OnDrawColor(CX(3), CY(9),  "G A M E   O V E R", RED);
		SCREEN->OnDraw(24, CY(11), ("SCORE  " + Pad(score, 6)).c_str());
		SCREEN->OnDraw(24, CY(12), ("HIGH   " + Pad(hiScore, 6)).c_str());
		SCREEN->OnDraw(22, CY(14), "PRESS SPACE TO RETRY");
		SCREEN->OnDraw(22, CY(15), "PRESS ESC TO TITLE");
	}
}

void PacManContent::DrawMaze()
{
	for (int r = 0; r < MAZE_H; r++)
	{
		for (int c = 0; c < MAZE_W; c++)
		{
			switch (tile[r][c])
			{
			case _ETile::WALL:  SCREEN->OnDrawColor(CX(c), CY(r), "■", BLUE); break;
			case _ETile::PELLET: SCREEN->OnDrawColor(CX(c), CY(r), "·", WHITE); break;
			case _ETile::POWER:
				if ((frameCount / 10) % 2 == 0)
					SCREEN->OnDrawColor(CX(c), CY(r), "●", WHITE);
				break;
			case _ETile::DOOR:  SCREEN->OnDrawColor(CX(c), CY(r), "≡", GRAY); break;
			default: break;
			}
		}
	}
}

void PacManContent::DrawGhosts()
{
	int base[4] = { RED, PURPLE, SKYBLUE, DARKYELLOW };	// Blinky/Pinky/Inky/Clyde
	for (int i = 0; i < 4; i++)
	{
		const Ghost& g = ghosts[i];
		if (g.state == _EGhostState::EATEN)
		{
			SCREEN->OnDrawColor(CX(g.x), CY(g.y), "◎", WHITE);	// 눈만
		}
		else if (g.state == _EGhostState::FRIGHTENED)
		{
			// 종료 직전 흰색 깜빡
			int col = (powerTimer < 120 && (frameCount / 6) % 2 == 0) ? WHITE : DARKBLUE;
			SCREEN->OnDrawColor(CX(g.x), CY(g.y), "Ω", col);
		}
		else
		{
			SCREEN->OnDrawColor(CX(g.x), CY(g.y), "Ω", base[i]);
		}
	}
}

void PacManContent::DrawPlayer()
{
	// 사망 연출 중 깜빡
	if (gameState == _EGameState::DYING && (dyingTimer / 4) % 2 == 0)
		return;
	SCREEN->OnDrawColor(CX(px), CY(py), PacGlyph(), YELLOW);
}

void PacManContent::DrawHUD()
{
	SCREEN->OnDrawColor(2, 1, "SCORE", WHITE);
	SCREEN->OnDrawColor(8, 1, Pad(score, 6).c_str(), YELLOW);

	SCREEN->OnDrawColor(18, 1, "HI", GRAY);
	SCREEN->OnDrawColor(21, 1, Pad(hiScore, 6).c_str(), GRAY);

	SCREEN->OnDrawColor(30, 1, "STAGE", WHITE);
	SCREEN->OnDrawColor(36, 1, std::to_string(stage).c_str(), SKYBLUE);

	SCREEN->OnDrawColor(42, 1, "LIFE", WHITE);
	for (int i = 0; i < life && i < 5; i++)
		SCREEN->OnDrawColor(48 + i * 2, 1, "♥", RED);

	if (powerTimer > 0)
	{
		int sec = powerTimer / 60 + 1;
		SCREEN->OnDrawColor(60, 1, ("POWER " + std::to_string(sec)).c_str(), SKYBLUE);
	}
}
