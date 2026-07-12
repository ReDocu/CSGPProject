#include "GalagaContent.h"
#include "../../framework.h"

// ============================================================================
// 튜닝 상수 (실제 플레이하며 조정)
// ============================================================================
static const float PLAYER_SPEED  = 0.8f;	// 플레이어 이동(칸/프레임)
static const float PBULLET_SPEED = 0.8f;	// 내 총알 속도(위로)
static const float EBULLET_SPEED = 0.35f;	// 적 총알 속도(아래로)
static const float FIRE_DELAY    = 0.18f;	// 발사 쿨다운(초)
static const float FX_TICK       = 0.06f;	// 폭발 애니 프레임 간격(초)
static const int   INV_FRAMES    = 90;		// 피격 후 무적 프레임(약 1.5초)

// zero-pad 문자열
static std::string Pad(int v, int width)
{
	std::string s = std::to_string(v);
	while ((int)s.size() < width)
		s = "0" + s;
	return s;
}

// ============================================================================
// 생명주기
// ============================================================================
void GalagaContent::OnInit()
{
	IGameContent::OnInit();		// currentPhase = TITLE, TIMER->StartContent()
	currentPhase = _EPhase::TITLE;

	hiScore = 0;

	pBullets.Clear();
	eBullets.Clear();
	enemies.Clear();
	fxs.Clear();

	px = (float)((PLAY_LEFT + PLAY_RIGHT) / 2);
	life = 3;
	score = 0;
	invincible = false;
	invTimer = 0;

	formX = (float)PLAY_LEFT;
	formY = 0;
	formDir = 1;
	formCols = 0;
	formRows = 0;
	stage = 1;
	aliveCount = 0;

	frameCount = 0;
	gameState = _EGameState::PLAYING;
}

void GalagaContent::OnRelease()
{
	IGameContent::OnRelease();
	pBullets.Clear();
	eBullets.Clear();
	enemies.Clear();
	fxs.Clear();
}

// 타이틀에서 게임을 새로 시작한다.
void GalagaContent::StartGame()
{
	pBullets.Clear();
	eBullets.Clear();
	enemies.Clear();
	fxs.Clear();

	px = (float)((PLAY_LEFT + PLAY_RIGHT) / 2);
	life = 3;
	score = 0;
	invincible = false;
	invTimer = 0;

	stage = 1;
	frameCount = 0;
	gameState = _EGameState::PLAYING;

	SpawnStage(stage);
	TIMER->StartContent();
}

// ============================================================================
// 스테이지 편대 스폰 (적 풀을 채운다 - 재할당 없이 Clear + Spawn)
// ============================================================================
void GalagaContent::SpawnStage(int s)
{
	enemies.Clear();

	// 편대 크기 (스테이지가 오를수록 확장)
	if (s == 1)      { formCols = 6; formRows = 2; }
	else if (s == 2) { formCols = 8; formRows = 2; }
	else if (s == 3) { formCols = 5; formRows = 4; }
	else
	{
		formCols = 6 + (s - 3);
		if (formCols > 8) formCols = 8;
		formRows = 4;
	}

	// 화면 중앙에 정렬
	int width = (formCols - 1) * SPACING_X;
	formX = (float)(PLAY_LEFT + ((PLAY_RIGHT - PLAY_LEFT) - width) / 2);
	if (formX < (float)PLAY_LEFT) formX = (float)PLAY_LEFT;
	formY = 0;
	formDir = 1;

	aliveCount = 0;
	for (int r = 0; r < formRows; r++)
	{
		for (int c = 0; c < formCols; c++)
		{
			int i = enemies.Spawn();
			if (i < 0) continue;

			Enemy e;
			e.col = c;
			e.row = r;

			// 종류: 윗줄일수록 강하게, 스테이지가 오를수록 상위 종류 등장
			if (r == 0 && s >= 3)
				e.type = _EEnemyType::STRONG;
			else if ((r % 2 == 0) && s >= 2)
				e.type = _EEnemyType::FAST;
			else
				e.type = _EEnemyType::BASIC;

			e.hp = (e.type == _EEnemyType::STRONG) ? ((s >= 5) ? 3 : 2) : 1;

			enemies.items[i] = e;
			aliveCount++;
		}
	}
}

// ============================================================================
// 타이틀 페이즈
// ============================================================================
static int s_titleSelect = 0;	// 0: 게임 시작, 1: 게임 종료

void GalagaContent::OnTitleUpdate()
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

void GalagaContent::OnTitleRender()
{
	SCREEN->OnDrawColor(5, 3, "■■■■■      ■      ■              ■      ■■■■■      ■", YELLOW);
	SCREEN->OnDrawColor(5, 4, "■            ■  ■    ■            ■  ■    ■            ■  ■", YELLOW);
	SCREEN->OnDrawColor(5, 5, "■  ■■■  ■■■■■  ■          ■■■■■  ■  ■■■  ■■■■■", YELLOW);
	SCREEN->OnDrawColor(5, 6, "■      ■  ■      ■  ■          ■      ■  ■      ■  ■      ■", YELLOW);
	SCREEN->OnDrawColor(5, 7, "■■■■■  ■      ■  ■■■■■  ■      ■  ■■■■■  ■      ■", YELLOW);

	// 장식용 적 편대
	int cols[6] = { RED, RED, GREEN, GREEN, PURPLE, PURPLE };
	for (int c = 0; c < 6; c++)
		SCREEN->OnDrawColor((13 + c * 3) * 2, 10, "▼", cols[c]);
	for (int c = 0; c < 5; c++)
		SCREEN->OnDrawColor((15 + c * 3) * 2, 12, "▼", RED);

	// 플레이어
	SCREEN->OnDrawColor(19 * 2, 16, "▲", SKYBLUE);

	if (hiScore > 0)
		SCREEN->OnDrawColor(30, 15, ("HI  " + Pad(hiScore, 6)).c_str(), GRAY);

	SCREEN->OnDrawColor(25, s_titleSelect == 0 ? 18 : 20, "▶", RED);
	SCREEN->OnDrawColor(32, 18, "게 임 시 작", BLUE);
	SCREEN->OnDrawColor(32, 20, "게 임 종 료", BLUE);

	SCREEN->OnDrawColor(14, 22, "A/D: 이동   SPACE: 발사   P: 일시정지   ESC: 타이틀로", GRAY);
	SCREEN->OnDrawColor(14, 23, "적 편대를 전멸시키면 다음 웨이브로 - 총알은 풀로 재사용된다", GRAY);
}

// ============================================================================
// 인게임 페이즈
// ============================================================================
void GalagaContent::OnInGameUpdate()
{
	// ESC -> 타이틀 (main 은 소비된 ESC 를 받아 종료하지 않음)
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

	if (gameState == _EGameState::STAGECLEAR)
	{
		if (INPUT->OnKeyDown(VK_SPACE) || INPUT->OnKeyDown(VK_RETURN))
		{
			stage++;
			pBullets.Clear();
			eBullets.Clear();
			fxs.Clear();
			SpawnStage(stage);
			gameState = _EGameState::PLAYING;
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
	UpdateFx();
	HandleCollisions();

	// 스테이지 클리어
	if (aliveCount <= 0)
	{
		score += 1000;	// 클리어 보너스
		if (score > hiScore) hiScore = score;
		gameState = _EGameState::STAGECLEAR;
		if (useBeep) Beep(1800, 30);
		return;
	}

	// 편대가 플레이어 라인까지 내려오면 게임 오버
	if (EnemiesBreached())
	{
		life = 0;
		if (score > hiScore) hiScore = score;
		gameState = _EGameState::GAMEOVER;
	}
}

// ============================================================================
// 플레이어 (이동 / 발사 / 무적)
// ============================================================================
void GalagaContent::UpdatePlayer()
{
	if (INPUT->OnKeyStay('A') || INPUT->OnKeyStay(VK_LEFT))
		px -= PLAYER_SPEED;
	if (INPUT->OnKeyStay('D') || INPUT->OnKeyStay(VK_RIGHT))
		px += PLAYER_SPEED;

	if (px < (float)PLAY_LEFT)  px = (float)PLAY_LEFT;
	if (px > (float)PLAY_RIGHT) px = (float)PLAY_RIGHT;

	// 발사: 스페이스 유지 + 쿨다운
	if (INPUT->OnKeyStay(VK_SPACE) && TIMER->GetTickTimer(FIRE_DELAY))
	{
		int i = pBullets.Spawn();
		if (i >= 0)
		{
			Bullet b;
			b.x = (float)((int)(px + 0.5f));
			b.y = (float)(PLAYER_Y - 1);
			b.vx = 0.0f;
			b.vy = -PBULLET_SPEED;
			pBullets.items[i] = b;
		}
	}

	if (invincible)
	{
		invTimer--;
		if (invTimer <= 0)
			invincible = false;
	}
}

// ============================================================================
// 적 편대 (좌우 행진 + 벽 반사 하강, 사격)
// ============================================================================
void GalagaContent::UpdateEnemies()
{
	// 편대 행진
	if (TIMER->GetTickTimer(FormTick()))
	{
		float next = formX + (float)formDir;
		int leftX = (int)(next + 0.5f);
		int rightX = leftX + (formCols - 1) * SPACING_X;

		if (leftX <= PLAY_LEFT || rightX >= PLAY_RIGHT)
		{
			formDir = -formDir;	// 방향 반전 + 한 칸 하강
			formY += 1;
		}
		else
		{
			formX = next;
		}
	}

	// 적 사격: 살아있는 적 중 랜덤 하나가 발사
	if (TIMER->GetTickTimer(AttackTick()))
	{
		int aliveIdx[MAX_ENEMY];
		int n = 0;
		for (int i = 0; i < MAX_ENEMY; i++)
			if (enemies.active[i])
				aliveIdx[n++] = i;

		if (n > 0)
		{
			const Enemy& e = enemies.items[aliveIdx[rand() % n]];
			int ex = EnemyX(e);
			int ey = EnemyY(e);

			// 강한 적은 3방향 확산(탄막), 그 외 1발
			int shots = (e.type == _EEnemyType::STRONG) ? 3 : 1;
			float vxs[3] = { 0.0f, -0.18f, 0.18f };
			for (int k = 0; k < shots; k++)
			{
				int bi = eBullets.Spawn();
				if (bi < 0) break;
				Bullet b;
				b.x = (float)ex;
				b.y = (float)(ey + 1);
				b.vx = vxs[k];
				b.vy = EBULLET_SPEED;
				eBullets.items[bi] = b;
			}
		}
	}
}

// ============================================================================
// 총알 이동 / 화면 밖 소멸
// ============================================================================
void GalagaContent::UpdateBullets()
{
	// 내 총알 (위로)
	for (int k = 0; k < MAX_PBULLET; k++)
	{
		if (!pBullets.active[k]) continue;
		pBullets.items[k].x += pBullets.items[k].vx;
		pBullets.items[k].y += pBullets.items[k].vy;
		if (pBullets.items[k].y < (float)HUD_Y + 1.0f)
			pBullets.Kill(k);
	}

	// 적 총알 (아래로)
	for (int k = 0; k < MAX_EBULLET; k++)
	{
		if (!eBullets.active[k]) continue;
		eBullets.items[k].x += eBullets.items[k].vx;
		eBullets.items[k].y += eBullets.items[k].vy;
		float bx = eBullets.items[k].x;
		float by = eBullets.items[k].y;
		if (by > (float)(PLAYER_Y + 1) || bx < (float)PLAY_LEFT || bx > (float)PLAY_RIGHT)
			eBullets.Kill(k);
	}
}

// ============================================================================
// 폭발 애니메이션
// ============================================================================
void GalagaContent::UpdateFx()
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

// ============================================================================
// 충돌 (풀 x 풀)
// ============================================================================
void GalagaContent::HandleCollisions()
{
	// 내 총알 -> 적
	for (int k = 0; k < MAX_PBULLET; k++)
	{
		if (!pBullets.active[k]) continue;
		int bx = (int)(pBullets.items[k].x + 0.5f);
		int by = (int)(pBullets.items[k].y + 0.5f);

		for (int i = 0; i < MAX_ENEMY; i++)
		{
			if (!enemies.active[i]) continue;
			Enemy& e = enemies.items[i];
			int ex = EnemyX(e);
			int ey = EnemyY(e);

			if (bx == ex && by == ey)
			{
				e.hp--;
				pBullets.Kill(k);

				if (e.hp <= 0)
				{
					SpawnFx((float)ex, (float)ey);
					score += ScoreOf(e.type);
					enemies.Kill(i);
					aliveCount--;
					if (useBeep) Beep(1200, 8);
				}
				else
				{
					if (useBeep) Beep(1500, 5);
				}
				break;	// 이 총알은 소비됨
			}
		}
	}

	// 적 총알 -> 나
	if (!invincible)
	{
		int prx = (int)(px + 0.5f);
		for (int k = 0; k < MAX_EBULLET; k++)
		{
			if (!eBullets.active[k]) continue;
			int bx = (int)(eBullets.items[k].x + 0.5f);
			int by = (int)(eBullets.items[k].y + 0.5f);
			if (bx == prx && by == PLAYER_Y)
			{
				eBullets.Kill(k);
				HitPlayer();
				break;
			}
		}
	}

	// 적 기체 -> 나 (같은 칸에서 충돌)
	if (!invincible)
	{
		int prx = (int)(px + 0.5f);
		for (int i = 0; i < MAX_ENEMY; i++)
		{
			if (!enemies.active[i]) continue;
			Enemy& e = enemies.items[i];
			int ex = EnemyX(e);
			int ey = EnemyY(e);
			if (ex == prx && ey == PLAYER_Y)
			{
				SpawnFx((float)ex, (float)ey);
				enemies.Kill(i);
				aliveCount--;
				HitPlayer();
				break;
			}
		}
	}
}

void GalagaContent::HitPlayer()
{
	life--;
	invincible = true;
	invTimer = INV_FRAMES;
	SpawnFx(px, (float)PLAYER_Y);
	if (useBeep) Beep(150, 40);

	if (life <= 0)
	{
		if (score > hiScore) hiScore = score;
		gameState = _EGameState::GAMEOVER;
	}
}

void GalagaContent::SpawnFx(float x, float y)
{
	int i = fxs.Spawn();
	if (i < 0) return;
	Fx f;
	f.x = x;
	f.y = y;
	f.frame = 0;
	fxs.items[i] = f;
}

bool GalagaContent::EnemiesBreached() const
{
	for (int i = 0; i < MAX_ENEMY; i++)
	{
		if (!enemies.active[i]) continue;
		if (EnemyY(enemies.items[i]) >= PLAYER_Y)
			return true;
	}
	return false;
}

// ============================================================================
// 헬퍼
// ============================================================================
float GalagaContent::FormTick() const
{
	switch (stage)
	{
	case 1:  return 0.50f;
	case 2:  return 0.42f;
	case 3:  return 0.34f;
	case 4:  return 0.28f;
	default: return 0.22f;
	}
}

float GalagaContent::AttackTick() const
{
	switch (stage)
	{
	case 1:  return 0.90f;
	case 2:  return 0.72f;
	case 3:  return 0.58f;
	case 4:  return 0.46f;
	default: return 0.38f;
	}
}

int GalagaContent::ScoreOf(_EEnemyType t) const
{
	switch (t)
	{
	case _EEnemyType::FAST:   return 200;
	case _EEnemyType::STRONG: return 500;
	default:                  return 100;
	}
}

int GalagaContent::EnemyX(const Enemy& e) const
{
	return (int)(formX + 0.5f) + e.col * SPACING_X;
}

int GalagaContent::EnemyY(const Enemy& e) const
{
	return FORM_TOP + formY + e.row * SPACING_Y;
}

// ============================================================================
// 렌더링
// ============================================================================
void GalagaContent::OnInGameRender()
{
	DrawStars();
	DrawEnemies();
	DrawBullets();
	DrawFx();
	DrawPlayer();
	DrawHUD();

	// 구분선 + 조작 안내
	std::string line(72, '=');
	SCREEN->OnDrawColor(4, DIVIDER_Y, line.c_str(), DARKGRAY);
	SCREEN->OnDraw(6, CONTROL_Y, "A/D 이동   SPACE 발사   P 정지   ESC 뒤로");

	if (gameState == _EGameState::PAUSE)
		SCREEN->OnDrawColor(34, 12, "-- PAUSE --", YELLOW);

	if (gameState == _EGameState::STAGECLEAR)
	{
		SCREEN->OnDrawColor(28, 10, ("STAGE " + std::to_string(stage) + " CLEAR").c_str(), YELLOW);
		SCREEN->OnDrawColor(32, 12, "BONUS +1000", GREEN);
		SCREEN->OnDraw(26, 15, "PRESS ENTER TO CONTINUE");
	}

	if (gameState == _EGameState::GAMEOVER)
	{
		SCREEN->OnDrawColor(30, 9, "G A M E   O V E R", RED);
		SCREEN->OnDraw(30, 11, ("SCORE  " + Pad(score, 6)).c_str());
		SCREEN->OnDraw(30, 12, ("HIGH   " + Pad(hiScore, 6)).c_str());
		SCREEN->OnDraw(30, 13, ("STAGE  " + std::to_string(stage)).c_str());
		SCREEN->OnDraw(28, 16, "PRESS SPACE TO RETRY");
		SCREEN->OnDraw(28, 17, "PRESS ESC TO TITLE");
	}
}

void GalagaContent::DrawStars()
{
	for (int i = 0; i < 18; i++)
	{
		int sx = (i * 7 + 3) % (PLAY_RIGHT - PLAY_LEFT) + PLAY_LEFT + 1;
		int sy = ((i * 5 + frameCount / 6) % 19) + 2;	// 2..20, 천천히 하강
		SCREEN->OnDrawColor(sx * 2, sy, ".", DARKGRAY);
	}
}

void GalagaContent::DrawEnemies()
{
	for (int i = 0; i < MAX_ENEMY; i++)
	{
		if (!enemies.active[i]) continue;
		const Enemy& e = enemies.items[i];
		int ex = EnemyX(e);
		int ey = EnemyY(e);
		if (ey < FORM_TOP || ey > PLAYER_Y) continue;

		int col;
		switch (e.type)
		{
		case _EEnemyType::FAST:   col = GREEN;  break;
		case _EEnemyType::STRONG: col = PURPLE; break;
		default:                  col = RED;    break;
		}
		SCREEN->OnDrawColor(ex * 2, ey, "▼", col);
	}
}

void GalagaContent::DrawBullets()
{
	// 적 총알 먼저(내 총알이 위에 보이도록)
	for (int k = 0; k < MAX_EBULLET; k++)
	{
		if (!eBullets.active[k]) continue;
		int bx = (int)(eBullets.items[k].x + 0.5f);
		int by = (int)(eBullets.items[k].y + 0.5f);
		if (by <= HUD_Y || by >= DIVIDER_Y) continue;
		SCREEN->OnDrawColor(bx * 2, by, "!", RED);
	}

	for (int k = 0; k < MAX_PBULLET; k++)
	{
		if (!pBullets.active[k]) continue;
		int bx = (int)(pBullets.items[k].x + 0.5f);
		int by = (int)(pBullets.items[k].y + 0.5f);
		if (by <= HUD_Y || by >= DIVIDER_Y) continue;
		SCREEN->OnDrawColor(bx * 2, by, "|", YELLOW);
	}
}

void GalagaContent::DrawFx()
{
	for (int k = 0; k < MAX_FX; k++)
	{
		if (!fxs.active[k]) continue;
		int fx = (int)(fxs.items[k].x + 0.5f);
		int fy = (int)(fxs.items[k].y + 0.5f);
		int fr = fxs.items[k].frame;

		const char* g = (fr < 2) ? "*" : (fr < 4) ? "×" : "+";
		int col = (fr < 4) ? YELLOW : DARKYELLOW;
		SCREEN->OnDrawColor(fx * 2, fy, g, col);
	}
}

void GalagaContent::DrawPlayer()
{
	// 무적 중 깜빡임
	if (invincible && (invTimer / 4) % 2 == 0)
		return;
	SCREEN->OnDrawColor((int)(px + 0.5f) * 2, PLAYER_Y, "▲", SKYBLUE);
}

void GalagaContent::DrawHUD()
{
	SCREEN->OnDrawColor(2, HUD_Y, "SCORE", WHITE);
	SCREEN->OnDrawColor(8, HUD_Y, Pad(score, 6).c_str(), YELLOW);

	SCREEN->OnDrawColor(17, HUD_Y, "HI", GRAY);
	SCREEN->OnDrawColor(20, HUD_Y, Pad(hiScore, 6).c_str(), GRAY);

	SCREEN->OnDrawColor(30, HUD_Y, "STAGE", WHITE);
	SCREEN->OnDrawColor(36, HUD_Y, std::to_string(stage).c_str(), SKYBLUE);

	SCREEN->OnDrawColor(42, HUD_Y, "LIFE", WHITE);
	for (int i = 0; i < life; i++)
		SCREEN->OnDrawColor(48 + i * 2, HUD_Y, "♥", RED);
}
