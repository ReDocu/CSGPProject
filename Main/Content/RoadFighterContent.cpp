#include "RoadFighterContent.h"
#include "../../framework.h"

// ============================================================================
// 튜닝 상수 (실제 플레이하며 조정)
// ============================================================================
static const float SPEED_MIN   = 80.0f;		// 최저 속도(km/h)
static const float SPEED_MAX   = 300.0f;	// 최고 속도
static const float ACCEL_RATE       = 2.5f;		// 가속/감속량(프레임당)
static const float SCROLL_K    = 0.001f;	// 속도 -> 셀/프레임 계수
static const float SCORE_HI    = 212.0f;	// 이 속도 이상이면 고속 보너스
static const float FUEL_DRAIN  = 0.035f;	// 연료 소모(프레임당, 저속 기준)

static const char* PLAYER_SPR[3] = { " X ", "XXX", " X " };	// 플레이어 십자 차량

// 8자리(또는 지정 폭) zero-pad 문자열
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
void RoadFighterContent::OnInit()
{
	IGameContent::OnInit();		// currentPhase = TITLE, TIMER->StartContent()
	currentPhase = _EPhase::TITLE;

	hiScore = 0;

	entities.clear();
	playerLane = 2;
	speed = SPEED_MIN;
	fuelF = 100.0f;
	life = 3;
	invincible = false;
	invTimer = 0;

	scrollAccum = 0.0f;
	roadScroll = 0.0f;
	score = 0;
	distance = 0;
	stage = 1;

	gameState = _EGameState::PLAYING;
}

void RoadFighterContent::OnRelease()
{
	IGameContent::OnRelease();
	entities.clear();
}

// 타이틀에서 게임을 새로 시작한다.
void RoadFighterContent::StartGame()
{
	entities.clear();
	playerLane = 2;
	speed = SPEED_MIN;
	fuelF = 100.0f;
	life = 3;
	invincible = false;
	invTimer = 0;

	scrollAccum = 0.0f;
	roadScroll = 0.0f;
	score = 0;
	distance = 0;
	stage = 1;

	gameState = _EGameState::PLAYING;
	TIMER->StartContent();
}

// ============================================================================
// 타이틀 페이즈
// ============================================================================
static int s_titleSelect = 0;	// 0: 게임 시작, 1: 게임 종료

void RoadFighterContent::OnTitleUpdate()
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

void RoadFighterContent::OnTitleRender()
{
	SCREEN->OnDrawColor(17, 2, "■■■■■  ■■■■■      ■      ■■■■", YELLOW);
	SCREEN->OnDrawColor(17, 3, "■      ■  ■      ■    ■  ■    ■      ■", YELLOW);
	SCREEN->OnDrawColor(17, 4, "■■■■■  ■      ■  ■■■■■  ■      ■", YELLOW);
	SCREEN->OnDrawColor(17, 5, "■  ■      ■      ■  ■      ■  ■      ■", YELLOW);
	SCREEN->OnDrawColor(17, 6, "■    ■■  ■■■■■  ■      ■  ■■■■", YELLOW);

	SCREEN->OnDrawColor(4, 8, "■■■■■ ■■■ ■■■■■ ■      ■ ■■■■■ ■■■■■ ■■■■■", YELLOW);
	SCREEN->OnDrawColor(4, 9, "■           ■   ■         ■      ■     ■     ■         ■      ■", YELLOW);
	SCREEN->OnDrawColor(4, 10, "■■■■     ■   ■  ■■■ ■■■■■     ■     ■■■■■ ■■■■■", YELLOW);
	SCREEN->OnDrawColor(4, 11, "■           ■   ■      ■ ■      ■     ■     ■         ■  ■", YELLOW);
	SCREEN->OnDrawColor(4, 12, "■         ■■■ ■■■■■ ■      ■     ■     ■■■■■ ■    ■■", YELLOW);

	SCREEN->OnDrawColor(25, s_titleSelect == 0 ? 15 : 18, "▶", RED);
	SCREEN->OnDrawColor(32, 15, "게 임 시 작", BLUE);
	SCREEN->OnDrawColor(32, 18, "게 임 종 료", BLUE);

	SCREEN->OnDrawColor(10, 21, "좌우: 차선 이동   위/아래: 가속/감속   P: 일시정지   ESC: 타이틀로", GRAY);
	SCREEN->OnDrawColor(10, 22, "◆ 연료   ★ 보너스   충돌 시 목숨 -1, 연료가 떨어지면 실패", GRAY);
}

// ============================================================================
// 인게임 페이즈
// ============================================================================
void RoadFighterContent::OnInGameUpdate()
{
	// ESC -> 타이틀
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
		if (INPUT->OnKeyDown(VK_SPACE) || INPUT->OnKeyDown(VK_RETURN))
			StartGame();
		return;
	}

	// ---- PLAYING ----
	UpdatePlayer();
	UpdateStage();
	UpdateWorld();
	UpdateFuel();
	HandleCollisions();

	if (life <= 0 || fuelF <= 0.0f)
	{
		gameState = _EGameState::GAMEOVER;
		if (score > hiScore)
			hiScore = score;
	}
}

// ============================================================================
// 플레이어 (차선 이동 / 가속·감속 / 무적)
// ============================================================================
void RoadFighterContent::UpdatePlayer()
{
	if (INPUT->OnKeyDown(VK_LEFT) && playerLane > 0)
		playerLane--;
	if (INPUT->OnKeyDown(VK_RIGHT) && playerLane < LANE_COUNT - 1)
		playerLane++;

	bool up = INPUT->OnKeyStay(VK_UP);
	bool dn = INPUT->OnKeyStay(VK_DOWN);
	if (up)
		speed += ACCEL_RATE;
	else if (dn)
		speed -= ACCEL_RATE;
	else
	{
		// 미입력 시 스테이지 기준 속도로 서서히 수렴
		float base = SPEED_MIN + (stage - 1) * 20.0f;
		if (speed > base) speed -= 0.5f;
		else if (speed < base) speed += 0.5f;
	}

	if (speed < SPEED_MIN) speed = SPEED_MIN;
	if (speed > SPEED_MAX) speed = SPEED_MAX;

	if (invincible)
	{
		invTimer--;
		if (invTimer <= 0)
			invincible = false;
	}
}

// ============================================================================
// 스테이지 (시간 기반)
// ============================================================================
void RoadFighterContent::UpdateStage()
{
	float t = TIMER->GetContentTime();
	if      (t < 30.0f) stage = 1;
	else if (t < 60.0f) stage = 2;
	else if (t < 90.0f) stage = 3;
	else if (t < 120.0f) stage = 4;
	else stage = 5;
}

// ============================================================================
// 월드 (스크롤 / 스폰 / 제거)
// ============================================================================
void RoadFighterContent::UpdateWorld()
{
	float step = speed * SCROLL_K;

	for (size_t i = 0; i < entities.size(); i++)
		entities[i].y += step;
	roadScroll += step;
	if (roadScroll > 1000.0f)
		roadScroll -= 1000.0f;

	// 거리/점수 누적
	scrollAccum += step;
	while (scrollAccum >= 1.0f)
	{
		scrollAccum -= 1.0f;
		distance += 1;
		score += (speed > SCORE_HI) ? 2 : 1;
	}

	// 화면 밖 제거 (적 차량은 통과 시 회피 점수)
	std::vector<Entity> kept;
	for (size_t i = 0; i < entities.size(); i++)
	{
		if (entities[i].y > (float)ROAD_BOTTOM)
		{
			_EEntity t = entities[i].type;
			if (t == _EEntity::ENEMY_NORMAL || t == _EEntity::ENEMY_FAST || t == _EEntity::TRUCK)
				score += 20;
		}
		else
		{
			kept.push_back(entities[i]);
		}
	}
	entities = kept;

	// 스폰 (스테이지별 간격)
	float interval;
	switch (stage)
	{
	case 1:  interval = 1.30f; break;
	case 2:  interval = 1.05f; break;
	case 3:  interval = 0.85f; break;
	case 4:  interval = 0.70f; break;
	default: interval = 0.60f; break;
	}
	if (TIMER->GetTickTimer(interval))
		SpawnEntity();

	// 고속 차량 위빙(차선 변경)
	if (TIMER->GetTickTimer(0.5f))
	{
		for (size_t i = 0; i < entities.size(); i++)
		{
			if (entities[i].type != _EEntity::ENEMY_FAST)
				continue;
			int dir = (rand() % 2) ? 1 : -1;
			int nl = entities[i].lane + dir;
			if (nl >= 0 && nl < LANE_COUNT)
				entities[i].lane = nl;
		}
	}
}

void RoadFighterContent::SpawnEntity()
{
	Entity e;
	e.y = (float)(ROAD_TOP - 3);	// 도로 위쪽 밖에서 슬라이드-인

	// 상단이 비어있는 차선 고르기(겹침 방지)
	int lane = rand() % LANE_COUNT;
	for (int tries = 0; tries < LANE_COUNT; tries++)
	{
		bool clear = true;
		for (size_t i = 0; i < entities.size(); i++)
			if (entities[i].lane == lane && entities[i].y < (float)(ROAD_TOP + 5))
			{
				clear = false;
				break;
			}
		if (clear) break;
		lane = (lane + 1) % LANE_COUNT;
	}
	e.lane = lane;

	int r = rand() % 100;
	if (r < 15)
	{
		e.type = _EEntity::FUEL;  e.w = 1; e.h = 1;
	}
	else if (r < 22)
	{
		e.type = _EEntity::BONUS; e.w = 1; e.h = 1;
	}
	else if (r < (stage >= 2 ? 28 : 22))
	{
		e.type = _EEntity::OBSTACLE; e.w = 1; e.h = 1;
	}
	else
	{
		int fastP = 0, truckP = 0;
		if (stage == 1)      { fastP = 0;  truckP = 0; }
		else if (stage == 2) { fastP = 30; truckP = 0; }
		else if (stage == 3) { fastP = 30; truckP = 15; }
		else                 { fastP = 35; truckP = 25; }

		int er = rand() % 100;
		if (er < truckP)              { e.type = _EEntity::TRUCK;        e.w = 4; e.h = 3; }
		else if (er < truckP + fastP) { e.type = _EEntity::ENEMY_FAST;   e.w = 2; e.h = 2; }
		else                          { e.type = _EEntity::ENEMY_NORMAL; e.w = 2; e.h = 2; }
	}

	entities.push_back(e);
}

// ============================================================================
// 연료
// ============================================================================
void RoadFighterContent::UpdateFuel()
{
	float factor = 1.0f + (speed - SPEED_MIN) / (SPEED_MAX - SPEED_MIN);	// 1.0 ~ 2.0
	fuelF -= FUEL_DRAIN * factor;
	if (fuelF < 0.0f)
		fuelF = 0.0f;
}

// ============================================================================
// 충돌 (아이템 획득 / 차량·장애물 피해)
// ============================================================================
void RoadFighterContent::HandleCollisions()
{
	int cx = LaneCenterX(playerLane);
	int pLeft = cx - 1, pRight = cx + 1;
	int pTop = PLAYER_Y - 2, pBottom = PLAYER_Y;

	std::vector<Entity> kept;
	for (size_t i = 0; i < entities.size(); i++)
	{
		Entity e = entities[i];
		int eLeft = LaneCenterX(e.lane) - e.w / 2;
		int eRight = eLeft + e.w - 1;
		int eTop = (int)(e.y + 0.5f);
		int eBottom = eTop + e.h - 1;

		bool hit = !(pRight < eLeft || pLeft > eRight || pBottom < eTop || pTop > eBottom);

		if (hit && e.type == _EEntity::FUEL)
		{
			fuelF += 25.0f;
			if (fuelF > 100.0f) fuelF = 100.0f;
			if (useBeep) Beep(1500, 20);
			continue;	// 소비
		}
		if (hit && e.type == _EEntity::BONUS)
		{
			score += 500;
			if (useBeep) Beep(1800, 20);
			continue;	// 소비
		}
		if (hit && !invincible)
		{
			// 적 차량 / 트럭 / 장애물 피해
			life -= 1;
			speed = SPEED_MIN;
			invincible = true;
			invTimer = 90;	// 약 1.5초 무적
		}

		kept.push_back(e);	// 차량/장애물은 유지(무적으로 재충돌 방지)
	}
	entities = kept;
}

int RoadFighterContent::LaneCenterX(int lane) const
{
	int laneW = (ROAD_RIGHT - ROAD_LEFT - 1) / LANE_COUNT;	// = 7
	return ROAD_LEFT + 1 + lane * laneW + laneW / 2;		// = 5 + 7*lane
}

// ============================================================================
// 렌더
// ============================================================================
void RoadFighterContent::OnInGameRender()
{
	DrawRoad();
	DrawEntities();
	DrawPlayer();
	DrawHUD();

	if (gameState == _EGameState::PAUSE)
		SCREEN->OnDrawColor(34, 12, "-- PAUSE --", YELLOW);

	if (gameState == _EGameState::GAMEOVER)
	{
		SCREEN->OnDrawColor(30, 8, "G A M E   O V E R", RED);
		SCREEN->OnDraw(30, 10, ("SCORE     " + Pad(score, 6)).c_str());
		SCREEN->OnDraw(30, 11, ("DISTANCE  " + std::to_string(distance) + "m").c_str());
		SCREEN->OnDraw(30, 12, ("HIGH      " + Pad(hiScore, 6)).c_str());
		SCREEN->OnDraw(28, 15, "PRESS SPACE TO RETRY");
		SCREEN->OnDraw(28, 16, "PRESS ESC TO TITLE");
	}
}

void RoadFighterContent::DrawRoad()
{
	// 상/하 경계
	for (int x = ROAD_LEFT; x <= ROAD_RIGHT; x++)
	{
		SCREEN->OnDrawColor(x * 2, ROAD_TOP - 1, "■", DARKGRAY);
		SCREEN->OnDrawColor(x * 2, ROAD_BOTTOM, "■", DARKGRAY);
	}

	// 좌우 벽
	for (int y = ROAD_TOP; y < ROAD_BOTTOM; y++)
	{
		SCREEN->OnDrawColor(ROAD_LEFT * 2, y, "■", DARKGRAY);
		SCREEN->OnDrawColor(ROAD_RIGHT * 2, y, "■", DARKGRAY);
	}

	// 차선 구분선(스크롤 점선)
	int laneW = (ROAD_RIGHT - ROAD_LEFT - 1) / LANE_COUNT;
	int off = (int)roadScroll;
	for (int i = 1; i < LANE_COUNT; i++)
	{
		int lx = ROAD_LEFT + 1 + i * laneW;
		for (int y = ROAD_TOP; y < ROAD_BOTTOM; y++)
			if (((y + off) % 3) != 0)
				SCREEN->OnDrawColor(lx * 2, y, "|", DARKGRAY);
	}
}

void RoadFighterContent::DrawEntities()
{
	for (size_t i = 0; i < entities.size(); i++)
	{
		Entity e = entities[i];
		int cx = LaneCenterX(e.lane);
		int left = cx - e.w / 2;
		int top = (int)(e.y + 0.5f);

		switch (e.type)
		{
		case _EEntity::ENEMY_NORMAL:
			DrawBlock(left, top, e.w, e.h, RED);
			break;
		case _EEntity::ENEMY_FAST:
			DrawBlock(left, top, e.w, e.h, PURPLE);
			break;
		case _EEntity::TRUCK:
			DrawBlock(left, top, e.w, e.h, DARKYELLOW);
			break;
		case _EEntity::FUEL:
			if (top >= ROAD_TOP && top < ROAD_BOTTOM)
				SCREEN->OnDrawColor(cx * 2, top, "◆", GREEN);
			break;
		case _EEntity::BONUS:
			if (top >= ROAD_TOP && top < ROAD_BOTTOM)
				SCREEN->OnDrawColor(cx * 2, top, "★", YELLOW);
			break;
		case _EEntity::OBSTACLE:
			if (top >= ROAD_TOP && top < ROAD_BOTTOM)
				SCREEN->OnDrawColor(cx * 2, top, "▲", DARKYELLOW);
			break;
		}
	}
}

void RoadFighterContent::DrawPlayer()
{
	// 무적 중 깜빡
	if (invincible && (invTimer / 4) % 2 == 0)
		return;

	int cx = LaneCenterX(playerLane);
	DrawSpriteRows(cx - 1, PLAYER_Y - 2, PLAYER_SPR, 3, SKYBLUE);
}

void RoadFighterContent::DrawHUD()
{
	// 상단
	SCREEN->OnDrawColor(2, 1, "SCORE", WHITE);
	SCREEN->OnDrawColor(8, 1, Pad(score, 8).c_str(), WHITE);

	SCREEN->OnDrawColor(30, 1, "STAGE", WHITE);
	SCREEN->OnDrawColor(36, 1, std::to_string(stage).c_str(), WHITE);

	SCREEN->OnDrawColor(44, 1, "HIGH", GRAY);
	SCREEN->OnDrawColor(49, 1, Pad(hiScore, 8).c_str(), GRAY);

	// 연료 바
	SCREEN->OnDrawColor(2, 2, "FUEL", WHITE);
	int f = (int)(fuelF + 0.5f);
	int filled = (f + 9) / 10;	// 0~10
	int fcol = (f > 60) ? GREEN : (f > 30 ? YELLOW : RED);
	for (int i = 0; i < 10; i++)
		SCREEN->OnDrawColor((4 + i) * 2, 2, "■", (i < filled) ? fcol : DARKGRAY);

	// 속도
	SCREEN->OnDrawColor(40, 2, "SPEED", WHITE);
	SCREEN->OnDrawColor(46, 2, (std::to_string((int)speed) + "km/h").c_str(), WHITE);

	// 하단 목숨
	SCREEN->OnDrawColor(2, 23, "LIFE", WHITE);
	for (int i = 0; i < life; i++)
		SCREEN->OnDrawColor(8 + i * 2, 23, "♥", RED);
}

void RoadFighterContent::DrawBlock(int left, int top, int w, int h, int color)
{
	for (int r = 0; r < h; r++)
	{
		int yy = top + r;
		if (yy < ROAD_TOP || yy >= ROAD_BOTTOM)
			continue;
		for (int c = 0; c < w; c++)
		{
			int xx = left + c;
			if (xx <= ROAD_LEFT || xx >= ROAD_RIGHT)
				continue;
			SCREEN->OnDrawColor(xx * 2, yy, "■", color);
		}
	}
}

void RoadFighterContent::DrawSpriteRows(int leftX, int topY, const char* const rows[], int h, int color)
{
	for (int r = 0; r < h; r++)
	{
		const char* row = rows[r];
		for (int c = 0; row[c] != '\0'; c++)
		{
			if (row[c] != ' ')
				SCREEN->OnDrawColor((leftX + c) * 2, topY + r, "■", color);
		}
	}
}
