#include "BattleCityContent.h"
#include "../../framework.h"
#include "../../FrameWork/UI/TextUtil.h"
using TextUtil::Pad;

// ============================================================================
// 튜닝 상수 (실제 플레이하며 조정). GetTickTimer 키는 서로 다른 값이어야 한다.
// ============================================================================
static const float PLAYER_MOVE  = 0.09f;	// 플레이어 이동 간격(초)
static const float BULLET_MOVE  = 0.045f;	// 총알 이동 간격(탱크보다 빠름)
static const float FIRE_DELAY   = 0.28f;	// 발사 쿨다운
static const float EMOVE_BASIC  = 0.16f;	// 적 종류별 이동 간격
static const float EMOVE_FAST   = 0.10f;
static const float EMOVE_ARMOR  = 0.22f;
static const float EMOVE_ATTACK = 0.13f;
static const float SPAWN_TICK   = 1.6f;		// 적 스폰 간격
static const float FX_TICK      = 0.06f;	// 폭발 애니 간격

static const int   INV_FRAMES     = 120;	// 피격 후 무적(약 2초)
static const int   STAGE_INTRO    = 84;		// STAGESTART 인트로(프레임)
static const int   HELMET_FRAMES  = 300;	// 헬멧 무적
static const int   FREEZE_FRAMES  = 360;	// 시계(적 정지)
static const int   SHIELD_FRAMES  = 600;	// 삽(기지 보호)

// Pad(): moved to FrameWork/UI/TextUtil.h

// ============================================================================
// 생명주기
// ============================================================================
void BattleCityContent::OnInit()
{
	IGameContent::OnInit();		// currentPhase = TITLE, TIMER->StartContent()
	currentPhase = _EPhase::TITLE;

	hiScore = 0;

	baseX = FIELD_W / 2;
	baseY = FIELD_H - 2;
	startX = baseX;
	startY = baseY - 2;

	score = 0;
	life = 3;
	powerLevel = 0;
	stage = 1;

	pBullets.Clear();
	eBullets.Clear();
	enemies.Clear();
	fxs.Clear();
	items.Clear();

	for (int y = 0; y < FIELD_H; y++)
		for (int x = 0; x < FIELD_W; x++)
			tile[y][x] = _ETile::EMPTY;

	baseAlive = true;
	shieldTimer = 0;
	freezeTimer = 0;
	enemiesToSpawn = 0;
	spawnCycle = 0;
	frameCount = 0;

	px = startX; py = startY; pdir = _EDir::UP;
	invincible = false; invTimer = 0;
	sliding = false; onIce = false; slideDir = _EDir::UP;

	gameState = _EGameState::PLAYING;
}

void BattleCityContent::OnRelease()
{
	IGameContent::OnRelease();
	pBullets.Clear();
	eBullets.Clear();
	enemies.Clear();
	fxs.Clear();
	items.Clear();
}

// 타이틀에서 게임을 새로 시작
void BattleCityContent::StartGame()
{
	score = 0;
	life = 3;
	powerLevel = 0;
	stage = 1;

	baseX = FIELD_W / 2;
	baseY = FIELD_H - 2;
	startX = baseX;
	startY = baseY - 2;

	StartStage(1);
}

// 스테이지 시작: 풀 비우기 + 맵 생성 + 상태 리셋
void BattleCityContent::StartStage(int s)
{
	pBullets.Clear();
	eBullets.Clear();
	enemies.Clear();
	fxs.Clear();
	items.Clear();

	GenerateMap(s);

	baseAlive = true;
	shieldTimer = 0;
	freezeTimer = 0;

	px = startX; py = startY; pdir = _EDir::UP;
	invincible = true; invTimer = 90;
	sliding = false; onIce = false;

	enemiesToSpawn = STAGE_TOTAL;
	spawnCycle = 0;
	frameCount = 0;

	gameState = _EGameState::STAGESTART;
	stageStartTimer = STAGE_INTRO;

	TIMER->StartContent();
}

// ============================================================================
// 절차적 맵 생성 (기술 축 2)
// ============================================================================
void BattleCityContent::GenerateMap(int s)
{
	for (int y = 0; y < FIELD_H; y++)
		for (int x = 0; x < FIELD_W; x++)
			tile[y][x] = _ETile::EMPTY;

	// 1) 장애물 (좌우 대칭 랜덤 배치)
	int brickN  = 7;
	int steelN  = 1 + s / 2;
	int waterN  = 1 + (rand() % 2) + (s >= 3 ? 1 : 0);
	int forestN = 2;
	int iceN    = (s >= 2) ? 2 : 1;

	for (int n = 0; n < brickN; n++)
	{
		int w = 1 + rand() % 3, h = 1 + rand() % 3;
		int x0 = 2 + rand() % (FIELD_W / 2 - 2);
		int y0 = 3 + rand() % (FIELD_H - 8);
		PlaceRectSym(_ETile::BRICK, x0, y0, w, h);
	}
	for (int n = 0; n < steelN; n++)
	{
		int x0 = 2 + rand() % (FIELD_W / 2 - 2);
		int y0 = 3 + rand() % (FIELD_H - 8);
		PlaceRectSym(_ETile::STEEL, x0, y0, 1 + rand() % 2, 1 + rand() % 2);
	}
	for (int n = 0; n < waterN; n++)
	{
		int x0 = 2 + rand() % (FIELD_W / 2 - 2);
		int y0 = 4 + rand() % (FIELD_H - 9);
		PlaceRectSym(_ETile::WATER, x0, y0, 2 + rand() % 2, 1 + rand() % 2);
	}
	for (int n = 0; n < forestN; n++)
	{
		int x0 = 2 + rand() % (FIELD_W / 2 - 2);
		int y0 = 4 + rand() % (FIELD_H - 9);
		PlaceRectSym(_ETile::FOREST, x0, y0, 2 + rand() % 3, 1 + rand() % 2);
	}
	for (int n = 0; n < iceN; n++)
	{
		int x0 = 2 + rand() % (FIELD_W / 2 - 2);
		int y0 = 4 + rand() % (FIELD_H - 9);
		PlaceRectSym(_ETile::ICE, x0, y0, 2 + rand() % 2, 1 + rand() % 2);
	}

	// 2) 철벽 테두리 (파괴 불가 경계)
	for (int x = 0; x < FIELD_W; x++)
	{
		tile[0][x] = _ETile::STEEL;
		tile[FIELD_H - 1][x] = _ETile::STEEL;
	}
	for (int y = 0; y < FIELD_H; y++)
	{
		tile[y][0] = _ETile::STEEL;
		tile[y][FIELD_W - 1] = _ETile::STEEL;
	}

	// 3) 기지 + 벽돌 요새
	tile[baseY][baseX] = _ETile::BASE;
	SetFortress(_ETile::BRICK);

	// 4) 스폰존/시작지점 확보 + 연결성 보정
	ClearSpawnZones();
	CarveConnectivity();

	// 5) 무결성 재적용 (터널이 요새/기지를 침범했을 수 있음)
	tile[baseY][baseX] = _ETile::BASE;
	SetFortress(_ETile::BRICK);
	ClearSpawnZones();
}

void BattleCityContent::PlaceRectSym(_ETile t, int x0, int y0, int w, int h)
{
	for (int yy = y0; yy < y0 + h; yy++)
	{
		for (int xx = x0; xx < x0 + w; xx++)
		{
			if (xx < 1 || xx > FIELD_W - 2 || yy < 1 || yy > FIELD_H - 2)
				continue;
			tile[yy][xx] = t;
			int mx = FIELD_W - 1 - xx;
			if (mx >= 1 && mx <= FIELD_W - 2)
				tile[yy][mx] = t;
		}
	}
}

void BattleCityContent::SetFortress(_ETile t)
{
	int fx[5] = { baseX - 1, baseX, baseX + 1, baseX - 1, baseX + 1 };
	int fy[5] = { baseY - 1, baseY - 1, baseY - 1, baseY, baseY };
	for (int k = 0; k < 5; k++)
	{
		if (!InField(fx[k], fy[k])) continue;
		if (fx[k] == baseX && fy[k] == baseY) continue;
		tile[fy[k]][fx[k]] = t;
	}
}

void BattleCityContent::ClearSpawnZones()
{
	int spawnTX[3] = { 2, FIELD_W / 2, FIELD_W - 3 };
	for (int s = 0; s < 3; s++)
	{
		int sx = spawnTX[s];
		for (int dy = 1; dy <= 2; dy++)
			if (InField(sx, dy))
				tile[dy][sx] = _ETile::EMPTY;
	}
	// 플레이어 시작 지점(기지 정면) 확보
	if (InField(startX, startY))     tile[startY][startX] = _ETile::EMPTY;
	if (InField(startX, startY - 1)) tile[startY - 1][startX] = _ETile::EMPTY;
}

// flood fill: 시작 지점에서 도달 못하는 스폰/기지정면을 터널로 뚫는다.
void BattleCityContent::CarveConnectivity()
{
	bool vis[FIELD_H][FIELD_W];
	for (int y = 0; y < FIELD_H; y++)
		for (int x = 0; x < FIELD_W; x++)
			vis[y][x] = false;

	int stackX[FIELD_W * FIELD_H];
	int stackY[FIELD_W * FIELD_H];
	int sp = 0;

	if (Passable(startX, startY))
	{
		vis[startY][startX] = true;
		stackX[sp] = startX; stackY[sp] = startY; sp++;
	}

	int dxs[4] = { 0, 0, -1, 1 };
	int dys[4] = { -1, 1, 0, 0 };
	while (sp > 0)
	{
		sp--;
		int cx = stackX[sp], cy = stackY[sp];
		for (int d = 0; d < 4; d++)
		{
			int nx = cx + dxs[d], ny = cy + dys[d];
			if (InField(nx, ny) && !vis[ny][nx] && Passable(nx, ny))
			{
				vis[ny][nx] = true;
				stackX[sp] = nx; stackY[sp] = ny; sp++;
			}
		}
	}

	// 시작 지점 방향으로 계단식 터널을 파서 연결
	int targetsX[4] = { 2, FIELD_W / 2, FIELD_W - 3, baseX };
	int targetsY[4] = { 1, 1, 1, baseY - 2 };
	for (int s = 0; s < 4; s++)
	{
		int cx = targetsX[s], cy = targetsY[s];
		int guard = 0;
		while (InField(cx, cy) && !vis[cy][cx] && guard++ < 300)
		{
			bool border = (cx == 0 || cy == 0 || cx == FIELD_W - 1 || cy == FIELD_H - 1);
			if (!border && !(cx == baseX && cy == baseY))
			{
				tile[cy][cx] = _ETile::EMPTY;
				vis[cy][cx] = true;
			}
			int ddx = startX - cx, ddy = startY - cy;
			if (ddx == 0 && ddy == 0) break;
			if (abs(ddx) >= abs(ddy) && ddx != 0) cx += (ddx > 0 ? 1 : -1);
			else                                  cy += (ddy > 0 ? 1 : -1);
			if (cx < 1) cx = 1; if (cx > FIELD_W - 2) cx = FIELD_W - 2;
			if (cy < 1) cy = 1; if (cy > FIELD_H - 2) cy = FIELD_H - 2;
		}
	}
}

// ============================================================================
// 격자/통행 판정
// ============================================================================
bool BattleCityContent::InField(int tx, int ty) const
{
	return (tx >= 0 && tx < FIELD_W && ty >= 0 && ty < FIELD_H);
}

bool BattleCityContent::Passable(int tx, int ty) const
{
	if (!InField(tx, ty)) return false;
	_ETile t = tile[ty][tx];
	return (t == _ETile::EMPTY || t == _ETile::FOREST || t == _ETile::ICE);
}

bool BattleCityContent::TankAt(int tx, int ty, int skipEnemy) const
{
	for (int i = 0; i < MAX_ENEMY; i++)
	{
		if (!enemies.active[i] || i == skipEnemy) continue;
		if (enemies.items[i].x == tx && enemies.items[i].y == ty)
			return true;
	}
	if (px == tx && py == ty) return true;
	return false;
}

// ============================================================================
// 타이틀 페이즈
// ============================================================================
static int s_titleSelect = 0;	// 0: 게임 시작, 1: 게임 종료

void BattleCityContent::OnTitleUpdate()
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

void BattleCityContent::OnTitleRender()
{
	SCREEN->OnDrawColor(5, 2, "■■■■        ■      ■■■■■  ■■■■■  ■          ■■■■■", YELLOW);
	SCREEN->OnDrawColor(5, 3, "■      ■    ■  ■        ■          ■      ■          ■", YELLOW);
	SCREEN->OnDrawColor(5, 4, "■■■■    ■■■■■      ■          ■      ■          ■■■■■", YELLOW);
	SCREEN->OnDrawColor(5, 5, "■      ■  ■      ■      ■          ■      ■          ■", YELLOW);
	SCREEN->OnDrawColor(5, 6, "■■■■    ■      ■      ■          ■      ■■■■■  ■■■■■", YELLOW);

	SCREEN->OnDrawColor(17, 8, "■■■■■  ■■■■■  ■■■■■  ■      ■", YELLOW);
	SCREEN->OnDrawColor(17, 9, "■              ■          ■        ■  ■", YELLOW);
	SCREEN->OnDrawColor(17, 10, "■              ■          ■          ■", YELLOW);
	SCREEN->OnDrawColor(17, 11, "■              ■          ■          ■", YELLOW);
	SCREEN->OnDrawColor(17, 12, "■■■■■  ■■■■■      ■          ■", YELLOW);

	// 장식: 기지와 이를 노리는 적 탱크
	SCREEN->OnDrawColor(24 * 2, 14, "▼", GRAY);
	SCREEN->OnDrawColor(27 * 2, 14, "▼", RED);
	SCREEN->OnDrawColor(30 * 2, 14, "▼", GREEN);
	for (int x = 24; x <= 30; x++)
		SCREEN->OnDrawColor(x * 2, 16, "□", DARKYELLOW);
	SCREEN->OnDrawColor(27 * 2, 16, "★", YELLOW);
	SCREEN->OnDrawColor(27 * 2, 15, "▲", YELLOW);

	if (hiScore > 0)
		SCREEN->OnDrawColor(4, 14, ("HI  " + Pad(hiScore, 6)).c_str(), GRAY);

	SCREEN->OnDrawColor(25, s_titleSelect == 0 ? 18 : 20, "▶", RED);
	SCREEN->OnDrawColor(32, 18, "게 임 시 작", BLUE);
	SCREEN->OnDrawColor(32, 20, "게 임 종 료", BLUE);

	SCREEN->OnDrawColor(12, 22, "WASD: 이동   SPACE: 발사   P: 일시정지   ESC: 타이틀로", GRAY);
	SCREEN->OnDrawColor(12, 23, "적 탱크를 전멸시키고 기지(★)를 지켜라", GRAY);
}

// ============================================================================
// 인게임 페이즈
// ============================================================================
void BattleCityContent::OnInGameUpdate()
{
	if (INPUT->OnKeyDown(VK_ESCAPE))
	{
		currentPhase = _EPhase::TITLE;
		return;
	}

	if (INPUT->OnKeyDown('P'))
	{
		if (gameState == _EGameState::PLAYING)       gameState = _EGameState::PAUSE;
		else if (gameState == _EGameState::PAUSE)    gameState = _EGameState::PLAYING;
	}

	if (gameState == _EGameState::STAGESTART)
	{
		stageStartTimer--;
		if (stageStartTimer <= 0 || INPUT->OnKeyDown(VK_SPACE) || INPUT->OnKeyDown(VK_RETURN))
			gameState = _EGameState::PLAYING;
		return;
	}
	if (gameState == _EGameState::PAUSE)
		return;
	if (gameState == _EGameState::STAGECLEAR)
	{
		if (INPUT->OnKeyDown(VK_SPACE) || INPUT->OnKeyDown(VK_RETURN))
		{
			stage++;
			StartStage(stage);
		}
		return;
	}
	if (gameState == _EGameState::GAMEOVER)
	{
		if (INPUT->OnKeyDown(VK_SPACE) || INPUT->OnKeyDown(VK_RETURN))
			StartGame();
		return;
	}

	// ---- PLAYING ----
	frameCount++;

	UpdatePlayer();
	UpdateEnemies();
	UpdateBullets();
	UpdateItems();
	UpdateFx();
	HandleCollisions();

	if (freezeTimer > 0) freezeTimer--;
	if (shieldTimer > 0)
	{
		shieldTimer--;
		if (shieldTimer == 0 && baseAlive)
			SetFortress(_ETile::BRICK);
	}

	// 패배: 기지 파괴 또는 생명 0
	if (!baseAlive)
	{
		if (score > hiScore) hiScore = score;
		gameState = _EGameState::GAMEOVER;
		if (useBeep) Beep(120, 200);
		return;
	}
	// (life<=0 는 HitPlayer 에서 GAMEOVER 로 전환)

	// 스테이지 클리어
	if (gameState == _EGameState::PLAYING && enemiesToSpawn <= 0 && ActiveEnemyCount() == 0)
	{
		score += 1000;
		if (score > hiScore) hiScore = score;
		gameState = _EGameState::STAGECLEAR;
		if (useBeep) Beep(1800, 40);
	}
}

// ============================================================================
// 플레이어 (이동/회전/발사/무적, 얼음 관성)
// ============================================================================
void BattleCityContent::UpdatePlayer()
{
	bool w = INPUT->OnKeyStay('W') || INPUT->OnKeyStay(VK_UP);
	bool s = INPUT->OnKeyStay('S') || INPUT->OnKeyStay(VK_DOWN);
	bool a = INPUT->OnKeyStay('A') || INPUT->OnKeyStay(VK_LEFT);
	bool d = INPUT->OnKeyStay('D') || INPUT->OnKeyStay(VK_RIGHT);

	bool wantMove = true;
	if      (w) pdir = _EDir::UP;
	else if (s) pdir = _EDir::DOWN;
	else if (a) pdir = _EDir::LEFT;
	else if (d) pdir = _EDir::RIGHT;
	else        wantMove = false;

	if (TIMER->GetTickTimer(PLAYER_MOVE))
	{
		if (wantMove)
		{
			int dx, dy; DirDelta(pdir, dx, dy);
			int nx = px + dx, ny = py + dy;
			if (Passable(nx, ny) && !TankAt(nx, ny, -1))
			{
				px = nx; py = ny;
			}
			onIce = (tile[py][px] == _ETile::ICE);
			sliding = onIce;
			slideDir = pdir;
		}
		else if (sliding && tile[py][px] == _ETile::ICE)
		{
			// 얼음 관성: 입력이 없어도 한 칸 미끄러진다
			int dx, dy; DirDelta(slideDir, dx, dy);
			int nx = px + dx, ny = py + dy;
			if (Passable(nx, ny) && !TankAt(nx, ny, -1))
			{
				px = nx; py = ny;
				sliding = (tile[py][px] == _ETile::ICE);
			}
			else sliding = false;
		}
		else sliding = false;
	}

	// 발사
	if (INPUT->OnKeyStay(VK_SPACE) && TIMER->GetTickTimer(FIRE_DELAY))
	{
		int maxB = (powerLevel >= 1) ? 2 : 1;
		int cur = 0;
		for (int k = 0; k < MAX_PBULLET; k++)
			if (pBullets.active[k]) cur++;
		if (cur < maxB)
			FireBullet(px, py, pdir, powerLevel >= 2, true);
	}

	if (invincible)
	{
		invTimer--;
		if (invTimer <= 0) invincible = false;
	}
}

// ============================================================================
// 적 탱크 (스폰 + AI 이동/사격)
// ============================================================================
void BattleCityContent::UpdateEnemies()
{
	if (TIMER->GetTickTimer(SPAWN_TICK))
	{
		if (ActiveEnemyCount() < SIMUL_MAX && enemiesToSpawn > 0)
			SpawnEnemy();
	}

	if (freezeTimer > 0)	// 시계 아이템: 기존 적 정지
		return;

	bool mv[4];
	mv[0] = TIMER->GetTickTimer(EMOVE_BASIC);
	mv[1] = TIMER->GetTickTimer(EMOVE_FAST);
	mv[2] = TIMER->GetTickTimer(EMOVE_ARMOR);
	mv[3] = TIMER->GetTickTimer(EMOVE_ATTACK);

	for (int i = 0; i < MAX_ENEMY; i++)
	{
		if (!enemies.active[i]) continue;
		Tank& e = enemies.items[i];

		e.dirTimer--;
		if (e.dirTimer <= 0)
		{
			ChooseEnemyDir(e);
			e.dirTimer = 30 + rand() % 60;
		}

		if (mv[(int)e.type])
		{
			int dx, dy; DirDelta(e.dir, dx, dy);
			int nx = e.x + dx, ny = e.y + dy;
			if (Passable(nx, ny) && !TankAt(nx, ny, i))
			{
				e.x = nx; e.y = ny;
			}
			else
			{
				ChooseEnemyDir(e);
				e.dirTimer = 30 + rand() % 60;
			}
		}

		e.fireTimer--;
		if (e.fireTimer <= 0)
		{
			FireBullet(e.x, e.y, e.dir, false, false);
			int base = (e.type == _EEnemyType::ATTACK) ? 45 : 90;
			e.fireTimer = base + rand() % base;
			if (useBeep) Beep(400, 4);
		}
	}
}

void BattleCityContent::ChooseEnemyDir(Tank& e)
{
	if (rand() % 100 < 40)
	{
		// 기지 방향 편향
		int ddx = baseX - e.x, ddy = baseY - e.y;
		if (abs(ddx) > abs(ddy)) e.dir = (ddx > 0) ? _EDir::RIGHT : _EDir::LEFT;
		else                     e.dir = (ddy > 0) ? _EDir::DOWN : _EDir::UP;
	}
	else
	{
		e.dir = (_EDir)(rand() % 4);
	}
}

void BattleCityContent::SpawnEnemy()
{
	int spawnTX[3] = { 2, FIELD_W / 2, FIELD_W - 3 };
	int s0 = rand() % 3;
	int chosen = -1;
	for (int t = 0; t < 3; t++)
	{
		int sx = spawnTX[(s0 + t) % 3];
		if (Passable(sx, 1) && !TankAt(sx, 1, -1)) { chosen = sx; break; }
	}
	if (chosen < 0) return;

	int idx = enemies.Spawn();
	if (idx < 0) return;

	Tank e;
	e.x = chosen; e.y = 1; e.dir = _EDir::DOWN;

	int r = rand() % 100;
	if (stage == 1)      e.type = (r < 80) ? _EEnemyType::BASIC : _EEnemyType::FAST;
	else if (stage == 2) e.type = (r < 55) ? _EEnemyType::BASIC : (r < 85) ? _EEnemyType::FAST : _EEnemyType::ATTACK;
	else if (stage == 3) e.type = (r < 40) ? _EEnemyType::BASIC : (r < 65) ? _EEnemyType::FAST : (r < 85) ? _EEnemyType::ATTACK : _EEnemyType::ARMOR;
	else                 e.type = (r < 30) ? _EEnemyType::BASIC : (r < 55) ? _EEnemyType::FAST : (r < 80) ? _EEnemyType::ATTACK : _EEnemyType::ARMOR;

	e.hp = (e.type == _EEnemyType::ARMOR) ? 4 : (e.type == _EEnemyType::ATTACK) ? 2 : 1;
	e.dirTimer = 20 + rand() % 40;
	e.fireTimer = 40 + rand() % 80;

	spawnCycle++;
	e.flashing = (spawnCycle % 4 == 0);	// 4번째마다 아이템 드랍 적

	enemies.items[idx] = e;
	enemiesToSpawn--;
}

// ============================================================================
// 총알 이동 / 타일 충돌
// ============================================================================
void BattleCityContent::UpdateBullets()
{
	if (!TIMER->GetTickTimer(BULLET_MOVE))
		return;

	for (int k = 0; k < MAX_PBULLET; k++)
		if (pBullets.active[k] && !StepBullet(pBullets.items[k]))
			pBullets.Kill(k);

	for (int k = 0; k < MAX_EBULLET; k++)
		if (eBullets.active[k] && !StepBullet(eBullets.items[k]))
			eBullets.Kill(k);
}

// 한 칸 전진 + 타일 명중 처리. false 반환 시 소멸.
bool BattleCityContent::StepBullet(Bullet& b)
{
	int dx, dy; DirDelta(b.dir, dx, dy);
	int nx = b.x + dx, ny = b.y + dy;
	if (!InField(nx, ny)) return false;

	_ETile t = tile[ny][nx];
	if (t == _ETile::BRICK)
	{
		tile[ny][nx] = _ETile::EMPTY;
		return false;
	}
	if (t == _ETile::STEEL)
	{
		bool border = (nx == 0 || ny == 0 || nx == FIELD_W - 1 || ny == FIELD_H - 1);
		if (b.power && !border)
			tile[ny][nx] = _ETile::EMPTY;
		return false;
	}
	if (t == _ETile::BASE)
	{
		baseAlive = false;
		SpawnFx(nx, ny);
		return false;
	}
	// EMPTY / WATER / FOREST / ICE -> 통과
	b.x = nx; b.y = ny;
	return true;
}

void BattleCityContent::FireBullet(int tx, int ty, _EDir dir, bool power, bool fromPlayer)
{
	if (!InField(tx, ty)) return;
	int idx = fromPlayer ? pBullets.Spawn() : eBullets.Spawn();
	if (idx < 0) return;

	Bullet b;
	b.x = tx; b.y = ty; b.dir = dir; b.power = power; b.fromPlayer = fromPlayer;
	if (fromPlayer) pBullets.items[idx] = b;
	else            eBullets.items[idx] = b;
}

// ============================================================================
// 아이템 / 폭발
// ============================================================================
void BattleCityContent::UpdateItems()
{
	for (int k = 0; k < MAX_ITEM; k++)
		if (items.active[k])
			items.items[k].blink++;
}

void BattleCityContent::UpdateFx()
{
	if (!TIMER->GetTickTimer(FX_TICK))
		return;
	for (int k = 0; k < MAX_FX; k++)
	{
		if (!fxs.active[k]) continue;
		fxs.items[k].frame++;
		if (fxs.items[k].frame >= 6)
			fxs.Kill(k);
	}
}

void BattleCityContent::SpawnFx(int tx, int ty)
{
	int i = fxs.Spawn();
	if (i < 0) return;
	Fx f; f.x = tx; f.y = ty; f.frame = 0;
	fxs.items[i] = f;
}

void BattleCityContent::DropItem(int tx, int ty)
{
	int i = items.Spawn();
	if (i < 0) return;
	Item it;
	it.type = (_EItemType)(rand() % 6);
	it.x = tx; it.y = ty; it.blink = 0;
	items.items[i] = it;
}

void BattleCityContent::ApplyItem(_EItemType type)
{
	switch (type)
	{
	case _EItemType::STAR:
		powerLevel++;
		if (powerLevel > 3) powerLevel = 3;
		break;
	case _EItemType::HELMET:
		invincible = true;
		invTimer = HELMET_FRAMES;
		break;
	case _EItemType::CLOCK:
		freezeTimer = FREEZE_FRAMES;
		break;
	case _EItemType::SHOVEL:
		shieldTimer = SHIELD_FRAMES;
		if (baseAlive) SetFortress(_ETile::STEEL);
		break;
	case _EItemType::LIFE:
		life++;
		break;
	case _EItemType::BOMB:
		for (int i = 0; i < MAX_ENEMY; i++)
		{
			if (!enemies.active[i]) continue;
			SpawnFx(enemies.items[i].x, enemies.items[i].y);
			score += ScoreOf(enemies.items[i].type);
			enemies.Kill(i);
		}
		break;
	}
}

// ============================================================================
// 충돌 (풀 x 풀)
// ============================================================================
void BattleCityContent::HandleCollisions()
{
	// 내 총알 -> 적 탱크
	for (int k = 0; k < MAX_PBULLET; k++)
	{
		if (!pBullets.active[k]) continue;
		Bullet& b = pBullets.items[k];
		for (int i = 0; i < MAX_ENEMY; i++)
		{
			if (!enemies.active[i]) continue;
			Tank& e = enemies.items[i];
			if (b.x == e.x && b.y == e.y)
			{
				e.hp--;
				pBullets.Kill(k);
				if (e.hp <= 0)
				{
					SpawnFx(e.x, e.y);
					score += ScoreOf(e.type);
					if (e.flashing) DropItem(e.x, e.y);
					enemies.Kill(i);
					if (useBeep) Beep(1200, 8);
				}
				break;
			}
		}
	}

	// 적 총알 -> 플레이어
	if (!invincible)
	{
		for (int k = 0; k < MAX_EBULLET; k++)
		{
			if (!eBullets.active[k]) continue;
			if (eBullets.items[k].x == px && eBullets.items[k].y == py)
			{
				eBullets.Kill(k);
				HitPlayer();
				break;
			}
		}
	}

	// 내 총알 x 적 총알 (상쇄)
	for (int k = 0; k < MAX_PBULLET; k++)
	{
		if (!pBullets.active[k]) continue;
		for (int j = 0; j < MAX_EBULLET; j++)
		{
			if (!eBullets.active[j]) continue;
			if (pBullets.items[k].x == eBullets.items[j].x &&
				pBullets.items[k].y == eBullets.items[j].y)
			{
				SpawnFx(pBullets.items[k].x, pBullets.items[k].y);
				pBullets.Kill(k);
				eBullets.Kill(j);
				break;
			}
		}
	}

	// 플레이어 -> 아이템 획득
	for (int k = 0; k < MAX_ITEM; k++)
	{
		if (!items.active[k]) continue;
		if (items.items[k].x == px && items.items[k].y == py)
		{
			ApplyItem(items.items[k].type);
			score += 500;
			items.Kill(k);
			if (useBeep) Beep(1600, 20);
		}
	}
}

void BattleCityContent::HitPlayer()
{
	life--;
	SpawnFx(px, py);
	if (useBeep) Beep(150, 40);

	if (life <= 0)
	{
		if (score > hiScore) hiScore = score;
		gameState = _EGameState::GAMEOVER;
		return;
	}

	powerLevel = 0;
	px = startX; py = startY; pdir = _EDir::UP;
	sliding = false; onIce = false;
	invincible = true; invTimer = INV_FRAMES;
}

int BattleCityContent::ScoreOf(_EEnemyType t) const
{
	switch (t)
	{
	case _EEnemyType::FAST:   return 200;
	case _EEnemyType::ATTACK: return 300;
	case _EEnemyType::ARMOR:  return 400;
	default:                  return 100;
	}
}

int BattleCityContent::ActiveEnemyCount() const
{
	int n = 0;
	for (int i = 0; i < MAX_ENEMY; i++)
		if (enemies.active[i]) n++;
	return n;
}

void BattleCityContent::DirDelta(_EDir dir, int& dx, int& dy) const
{
	switch (dir)
	{
	case _EDir::UP:    dx = 0;  dy = -1; break;
	case _EDir::RIGHT: dx = 1;  dy = 0;  break;
	case _EDir::DOWN:  dx = 0;  dy = 1;  break;
	default:           dx = -1; dy = 0;  break;
	}
}

const char* BattleCityContent::TankGlyph(_EDir dir) const
{
	switch (dir)
	{
	case _EDir::UP:    return "▲";
	case _EDir::RIGHT: return "▶";
	case _EDir::DOWN:  return "▼";
	default:           return "◀";
	}
}

// ============================================================================
// 렌더링
// ============================================================================
void BattleCityContent::OnInGameRender()
{
	DrawMap();
	DrawItemsLayer();
	DrawTanks();
	DrawBullets();
	DrawFx();
	DrawForestOverlay();	// 숲을 위에 다시 덮어 시야 가림
	DrawHUD();

	SCREEN->OnDraw(2, 23, "WASD 이동  SPACE 발사  P 정지  ESC 뒤로");

	if (gameState == _EGameState::STAGESTART)
		SCREEN->OnDrawColor(20, 11, ("S T A G E   " + std::to_string(stage)).c_str(), WHITE);

	if (gameState == _EGameState::PAUSE)
		SCREEN->OnDrawColor(22, 11, "-- PAUSE --", YELLOW);

	if (gameState == _EGameState::STAGECLEAR)
	{
		SCREEN->OnDrawColor(20, 10, ("STAGE " + std::to_string(stage) + " CLEAR").c_str(), YELLOW);
		SCREEN->OnDrawColor(23, 12, "BONUS +1000", GREEN);
		SCREEN->OnDraw(18, 14, "PRESS ENTER TO CONTINUE");
	}

	if (gameState == _EGameState::GAMEOVER)
	{
		SCREEN->OnDrawColor(21, 9, "G A M E   O V E R", RED);
		if (!baseAlive)
			SCREEN->OnDrawColor(22, 10, "BASE DESTROYED", RED);
		SCREEN->OnDraw(22, 12, ("SCORE  " + Pad(score, 6)).c_str());
		SCREEN->OnDraw(22, 13, ("HIGH   " + Pad(hiScore, 6)).c_str());
		SCREEN->OnDraw(20, 15, "PRESS SPACE TO RETRY");
		SCREEN->OnDraw(20, 16, "PRESS ESC TO TITLE");
	}
}

void BattleCityContent::DrawMap()
{
	for (int ty = 0; ty < FIELD_H; ty++)
	{
		for (int tx = 0; tx < FIELD_W; tx++)
		{
			switch (tile[ty][tx])
			{
			case _ETile::EMPTY:  break;
			case _ETile::BRICK:  SCREEN->OnDrawColor(TX(tx), TY(ty), "□", DARKYELLOW); break;
			case _ETile::STEEL:  SCREEN->OnDrawColor(TX(tx), TY(ty), "■", GRAY); break;
			case _ETile::WATER:  SCREEN->OnDrawColor(TX(tx), TY(ty), "▒", SKYBLUE); break;
			case _ETile::FOREST: SCREEN->OnDrawColor(TX(tx), TY(ty), "♣", DARKGREEN); break;
			case _ETile::ICE:    SCREEN->OnDrawColor(TX(tx), TY(ty), "≡", WHITE); break;
			case _ETile::BASE:
				if (baseAlive) SCREEN->OnDrawColor(TX(tx), TY(ty), "★", YELLOW);
				else           SCREEN->OnDrawColor(TX(tx), TY(ty), "X", DARKRED);
				break;
			}
		}
	}
}

void BattleCityContent::DrawItemsLayer()
{
	for (int k = 0; k < MAX_ITEM; k++)
	{
		if (!items.active[k]) continue;
		Item& it = items.items[k];
		if ((it.blink / 6) % 2 == 0) continue;	// 깜빡임

		const char* g = "◆";
		int col = WHITE;
		switch (it.type)
		{
		case _EItemType::STAR:   g = "★"; col = SKYBLUE; break;
		case _EItemType::HELMET: g = "◆"; col = SKYBLUE; break;
		case _EItemType::CLOCK:  g = "◎"; col = WHITE;   break;
		case _EItemType::SHOVEL: g = "▨"; col = DARKYELLOW; break;
		case _EItemType::LIFE:   g = "♥"; col = GREEN;   break;
		case _EItemType::BOMB:   g = "●"; col = RED;     break;
		}
		SCREEN->OnDrawColor(TX(it.x), TY(it.y), g, col);
	}
}

void BattleCityContent::DrawTanks()
{
	// 적
	for (int i = 0; i < MAX_ENEMY; i++)
	{
		if (!enemies.active[i]) continue;
		Tank& e = enemies.items[i];
		int col;
		switch (e.type)
		{
		case _EEnemyType::FAST:   col = SKYBLUE; break;
		case _EEnemyType::ARMOR:  col = GREEN;   break;
		case _EEnemyType::ATTACK: col = RED;     break;
		default:                  col = GRAY;    break;
		}
		if (e.flashing && (frameCount / 6) % 2 == 0)
			col = WHITE;
		SCREEN->OnDrawColor(TX(e.x), TY(e.y), TankGlyph(e.dir), col);
	}

	// 플레이어 (무적 중 깜빡)
	if (!(invincible && (invTimer / 4) % 2 == 0))
		SCREEN->OnDrawColor(TX(px), TY(py), TankGlyph(pdir), YELLOW);
}

void BattleCityContent::DrawBullets()
{
	for (int k = 0; k < MAX_PBULLET; k++)
		if (pBullets.active[k])
			SCREEN->OnDrawColor(TX(pBullets.items[k].x), TY(pBullets.items[k].y), "●", WHITE);

	for (int k = 0; k < MAX_EBULLET; k++)
		if (eBullets.active[k])
			SCREEN->OnDrawColor(TX(eBullets.items[k].x), TY(eBullets.items[k].y), "●", RED);
}

void BattleCityContent::DrawFx()
{
	for (int k = 0; k < MAX_FX; k++)
	{
		if (!fxs.active[k]) continue;
		int fr = fxs.items[k].frame;
		const char* g = (fr < 2) ? "*" : (fr < 4) ? "+" : "X";
		int col = (fr < 4) ? YELLOW : RED;
		SCREEN->OnDrawColor(TX(fxs.items[k].x), TY(fxs.items[k].y), g, col);
	}
}

void BattleCityContent::DrawForestOverlay()
{
	for (int ty = 0; ty < FIELD_H; ty++)
		for (int tx = 0; tx < FIELD_W; tx++)
			if (tile[ty][tx] == _ETile::FOREST)
				SCREEN->OnDrawColor(TX(tx), TY(ty), "♣", DARKGREEN);
}

void BattleCityContent::DrawHUD()
{
	// 상단
	SCREEN->OnDrawColor(2, 1, "SCORE", WHITE);
	SCREEN->OnDrawColor(8, 1, Pad(score, 6).c_str(), YELLOW);

	SCREEN->OnDrawColor(20, 1, "LIFE", WHITE);
	for (int i = 0; i < life && i < 5; i++)
		SCREEN->OnDrawColor(26 + i * 2, 1, "♥", RED);

	// 우측 패널
	int hx = 52;
	int remain = enemiesToSpawn + ActiveEnemyCount();
	SCREEN->OnDrawColor(hx, 3, "ENEMY", WHITE);
	SCREEN->OnDrawColor(hx + 6, 3, std::to_string(remain).c_str(), RED);

	SCREEN->OnDrawColor(hx, 5, "STAGE", WHITE);
	SCREEN->OnDrawColor(hx + 6, 5, std::to_string(stage).c_str(), SKYBLUE);

	SCREEN->OnDrawColor(hx, 7, "POWER", WHITE);
	SCREEN->OnDrawColor(hx + 6, 7, std::to_string(powerLevel).c_str(), YELLOW);

	SCREEN->OnDrawColor(hx, 9, "HI", GRAY);
	SCREEN->OnDrawColor(hx + 3, 9, Pad(hiScore, 6).c_str(), GRAY);

	if (freezeTimer > 0) SCREEN->OnDrawColor(hx, 12, "FREEZE", SKYBLUE);
	if (shieldTimer > 0) SCREEN->OnDrawColor(hx, 13, "SHIELD", GREEN);
}
