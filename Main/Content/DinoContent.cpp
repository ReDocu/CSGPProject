#include "DinoContent.h"
#include "../../framework.h"

// ============================================================================
// 튜닝 상수 (칸/프레임 단위). 실제 플레이하며 조정한다.
// ============================================================================
static const float G           = 0.06f;		// 중력
static const float JUMP_V      = -0.95f;	// 점프 초속(위로 음수)
static const float FASTFALL    = 3.0f;		// 빠른 착지 중력 배수
static const float SPEED_0     = 0.45f;		// 시작 속도
static const float SPEED_MAX   = 1.2f;		// 최대 속도
static const float SPEED_ACCEL = 0.0004f;	// 점수당 속도 증가
static const float SCORE_K     = 2.0f;		// 프레임당 점수 계수

// ============================================================================
// 스프라이트 (전각 블록 근사, 'X'=블록 ' '=여백)
// ============================================================================
static const char* DINO_RUN0[3] = { " XX", "XXX", "X X" };	// 달리기 프레임 0
static const char* DINO_RUN1[3] = { " XX", "XXX", "XX " };	// 달리기 프레임 1
static const char* DINO_JUMP[3] = { " XX", "XXX", "X X" };	// 점프
static const char* DINO_DEAD[3] = { " XX", "XX ", "X X" };	// 사망
static const char* DINO_DUCK[2] = { " XXX", "XXXX" };		// 숙이기(납작)

static const char* CACTUS_S[2]  = { "X", "X" };				// 작은 선인장
static const char* CACTUS_L[3]  = { "X X", "XXX", " X " };	// 큰 선인장
static const char* PTERO_A[2]   = { "X  ", "XXX" };			// 익룡 날개 위
static const char* PTERO_B[2]   = { "XXX", " X " };			// 익룡 날개 아래

// ============================================================================
// 생명주기
// ============================================================================
void DinoContent::OnInit()
{
	IGameContent::OnInit();		// currentPhase = TITLE, TIMER->StartContent()
	currentPhase = _EPhase::TITLE;

	hiScore = 0;
	gameState = _EGameState::PLAYING;

	dinoY = (float)DINO_FOOT;
	dinoVelY = 0.0f;
	dinoState = _EDinoState::RUN;
	onGround = true;

	obstacles.clear();
	clouds.clear();

	gameSpeed = SPEED_0;
	spawnGap = 30.0f;
	groundScrollX = 0.0f;

	scoreF = 0.0f;
	score = 0;
	nightPhase = 0;
	scoreBlink = 0;
	legFrame = 0;
	wingFrame = 0;
}

void DinoContent::OnRelease()
{
	IGameContent::OnRelease();
	obstacles.clear();
	clouds.clear();
}

// 타이틀에서 게임을 새로 시작한다.
void DinoContent::StartGame()
{
	obstacles.clear();
	clouds.clear();

	dinoY = (float)DINO_FOOT;
	dinoVelY = 0.0f;
	dinoState = _EDinoState::RUN;
	onGround = true;

	gameSpeed = SPEED_0;
	scoreF = 0.0f;
	score = 0;
	nightPhase = 0;
	spawnGap = 30.0f;		// 시작 직후 잠깐의 여유(그레이스)
	groundScrollX = 0.0f;
	scoreBlink = 0;
	legFrame = 0;
	wingFrame = 0;

	gameState = _EGameState::PLAYING;
	TIMER->StartContent();
}

// ============================================================================
// 타이틀 페이즈
// ============================================================================
static int s_titleSelect = 0;	// 0: 게임 시작, 1: 게임 종료

void DinoContent::OnTitleUpdate()
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

void DinoContent::OnTitleRender()
{
	SCREEN->OnDrawColor(17, 2, "■■■■    ■■■■■  ■      ■  ■■■■■", GREEN);
	SCREEN->OnDrawColor(17, 3, "■      ■      ■      ■    ■■  ■      ■", GREEN);
	SCREEN->OnDrawColor(17, 4, "■      ■      ■      ■  ■  ■  ■      ■", GREEN);
	SCREEN->OnDrawColor(17, 5, "■      ■      ■      ■■    ■  ■      ■", GREEN);
	SCREEN->OnDrawColor(17, 6, "■■■■    ■■■■■  ■      ■  ■■■■■", GREEN);

	SCREEN->OnDrawColor(23, 8, "■■■■■  ■      ■  ■      ■", GREEN);
	SCREEN->OnDrawColor(23, 9, "■      ■  ■      ■  ■    ■■", GREEN);
	SCREEN->OnDrawColor(23, 10, "■■■■■  ■      ■  ■  ■  ■", GREEN);
	SCREEN->OnDrawColor(23, 11, "■  ■      ■      ■  ■■    ■", GREEN);
	SCREEN->OnDrawColor(23, 12, "■    ■■  ■■■■■  ■      ■", GREEN);

	// 데모용 지면 + 서 있는 공룡
	for (int x = 10; x <= 30; x++)
		SCREEN->OnDrawColor(x * 2, 17, "■", GRAY);
	DrawSpriteRows(13, 14, DINO_RUN0, 3, WHITE);

	SCREEN->OnDrawColor(25, s_titleSelect == 0 ? 19 : 21, "▶", RED);
	SCREEN->OnDrawColor(32, 19, "게 임 시 작", BLUE);
	SCREEN->OnDrawColor(32, 21, "게 임 종 료", BLUE);

	SCREEN->OnDrawColor(10, 23, "점프: SPACE/위   숙이기: 아래   P: 일시정지   ESC: 타이틀로", GRAY);
}

// ============================================================================
// 인게임 페이즈
// ============================================================================
void DinoContent::OnInGameUpdate()
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
	if (TIMER->GetTickTimer(0.1f))  legFrame ^= 1;
	if (TIMER->GetTickTimer(0.15f)) wingFrame ^= 1;
	if (scoreBlink > 0) scoreBlink--;

	UpdateDino();
	UpdateWorld();
	UpdateScore();

	if (CheckCollision())
	{
		dinoState = _EDinoState::DEAD;
		gameState = _EGameState::GAMEOVER;
		if (score > hiScore)
			hiScore = score;
	}
}

// ============================================================================
// 공룡 물리
// ============================================================================
void DinoContent::UpdateDino()
{
	bool sp = INPUT->OnKeyDown(VK_SPACE);
	bool up = INPUT->OnKeyDown(VK_UP);
	bool jumpEdge = sp || up;
	bool downStay = INPUT->OnKeyStay(VK_DOWN);

	if (onGround)
	{
		if (jumpEdge)
		{
			dinoVelY = JUMP_V;
			onGround = false;
			dinoState = _EDinoState::JUMP;
		}
		else
		{
			dinoState = downStay ? _EDinoState::DUCK : _EDinoState::RUN;
		}
	}

	if (!onGround)
	{
		float g = G;
		if (downStay) g *= FASTFALL;	// 빠른 착지

		dinoVelY += g;
		dinoY += dinoVelY;

		if (dinoY >= (float)DINO_FOOT)
		{
			dinoY = (float)DINO_FOOT;
			dinoVelY = 0.0f;
			onGround = true;
			dinoState = downStay ? _EDinoState::DUCK : _EDinoState::RUN;
		}
		else
		{
			dinoState = _EDinoState::JUMP;
		}
	}
}

// ============================================================================
// 월드 (스크롤 / 스폰 / 속도)
// ============================================================================
void DinoContent::UpdateWorld()
{
	// 속도 가속
	gameSpeed = SPEED_0 + score * SPEED_ACCEL;
	if (gameSpeed > SPEED_MAX)
		gameSpeed = SPEED_MAX;

	// 스크롤
	for (size_t i = 0; i < obstacles.size(); i++)
		obstacles[i].x -= gameSpeed;
	for (size_t i = 0; i < clouds.size(); i++)
		clouds[i].x -= gameSpeed * 0.3f;	// 시차(느리게)

	groundScrollX += gameSpeed;
	if (groundScrollX > 1000.0f)
		groundScrollX -= 1000.0f;

	// 화면 밖 제거
	std::vector<Obstacle> keptO;
	for (size_t i = 0; i < obstacles.size(); i++)
		if (obstacles[i].x + obstacles[i].w >= 0.0f)
			keptO.push_back(obstacles[i]);
	obstacles = keptO;

	std::vector<Cloud> keptC;
	for (size_t i = 0; i < clouds.size(); i++)
		if (clouds[i].x + 3 >= 0.0f)
			keptC.push_back(clouds[i]);
	clouds = keptC;

	// 장애물 스폰
	spawnGap -= gameSpeed;
	if (spawnGap <= 0.0f)
		SpawnObstacle();

	// 구름 스폰(가끔)
	if ((int)clouds.size() < 3 && (rand() % 120) == 0)
		SpawnCloud();
}

void DinoContent::SpawnObstacle()
{
	Obstacle o;
	o.x = 38.0f;

	int roll = rand() % 100;
	if (score < 200 || roll < 70)
	{
		// 선인장 (200점 전에는 선인장만)
		if (rand() % 2 == 0)
		{
			o.type = _EObstacle::CACTUS_SMALL;
			o.w = 1; o.h = 2; o.topY = GROUND_Y - 2;	// y 17~18
		}
		else
		{
			o.type = _EObstacle::CACTUS_LARGE;
			o.w = 3; o.h = 3; o.topY = GROUND_Y - 3;	// y 16~18
		}
	}
	else
	{
		// 익룡 (3높이)
		int lvl = rand() % 3;
		o.w = 3; o.h = 2;
		if (lvl == 0)      { o.type = _EObstacle::PTERO_LOW;  o.topY = 17; }	// 점프 필수
		else if (lvl == 1) { o.type = _EObstacle::PTERO_MID;  o.topY = 15; }	// 숙이기/점프
		else               { o.type = _EObstacle::PTERO_HIGH; o.topY = 13; }	// 서 있으면 안전
	}
	obstacles.push_back(o);

	// 다음 스폰까지의 최소 간격(점프로 넘을 수 있도록 속도 비례) + 랜덤
	float gapMin = 14.0f + 18.0f * gameSpeed;
	spawnGap = gapMin + (float)(rand() % 10);
}

void DinoContent::SpawnCloud()
{
	Cloud c;
	c.x = 39.0f;
	c.y = 2 + rand() % 4;	// y 2~5
	clouds.push_back(c);
}

// ============================================================================
// 점수 / 마일스톤 / 밤낮
// ============================================================================
void DinoContent::UpdateScore()
{
	int prevHundred = score / 100;

	scoreF += gameSpeed * SCORE_K;
	score = (int)scoreF;

	// 100점 단위 도달 시 점수 깜빡 + 소리
	if (score / 100 > prevHundred)
	{
		scoreBlink = 12;
		if (useBeep)
			Beep(1200, 20);
	}

	// 700점마다 밤 <-> 낮
	nightPhase = (score / 700) % 2;
}

// ============================================================================
// 충돌 (AABB)
// ============================================================================
bool DinoContent::CheckCollision()
{
	int footY = (int)(dinoY + 0.5f);

	int dl, dr, dt, db;
	if (dinoState == _EDinoState::DUCK)
	{
		dl = DINO_X; dr = DINO_X + 3;
		dt = footY - 1; db = footY;
	}
	else
	{
		dl = DINO_X; dr = DINO_X + 2;
		dt = footY - 2; db = footY;
	}
	dl += 1;	// 좌측을 한 칸 안쪽으로(플레이어에게 관대하게)

	for (size_t i = 0; i < obstacles.size(); i++)
	{
		int oLeft   = (int)(obstacles[i].x + 0.5f);
		int oRight  = oLeft + obstacles[i].w - 1;
		int oTop    = obstacles[i].topY;
		int oBottom = obstacles[i].topY + obstacles[i].h - 1;

		if (!(dr < oLeft || dl > oRight || db < oTop || dt > oBottom))
			return true;
	}
	return false;
}

// ============================================================================
// 렌더
// ============================================================================
void DinoContent::OnInGameRender()
{
	DrawBackground();
	DrawGround();

	for (size_t i = 0; i < obstacles.size(); i++)
		DrawObstacle(obstacles[i]);

	DrawDino();
	DrawScore();

	if (gameState == _EGameState::PAUSE)
		SCREEN->OnDrawColor(34, 10, "-- PAUSE --", YELLOW);

	if (gameState == _EGameState::GAMEOVER)
	{
		SCREEN->OnDrawColor(32, 9, "G A M E   O V E R", RED);
		SCREEN->OnDraw(30, 11, "PRESS SPACE TO RESTART");
	}
}

int DinoContent::ForeColor() const
{
	return (nightPhase == 1) ? DARKGRAY : GRAY;
}

void DinoContent::DrawSpriteRows(int leftX, int topY, const char* const rows[], int h, int color)
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

void DinoContent::DrawDino()
{
	int footY = (int)(dinoY + 0.5f);
	int dinoColor = (nightPhase == 1) ? GRAY : WHITE;

	if (dinoState == _EDinoState::DUCK)
		DrawSpriteRows(DINO_X, footY - 1, DINO_DUCK, 2, dinoColor);
	else if (dinoState == _EDinoState::JUMP)
		DrawSpriteRows(DINO_X, footY - 2, DINO_JUMP, 3, dinoColor);
	else if (dinoState == _EDinoState::DEAD)
		DrawSpriteRows(DINO_X, footY - 2, DINO_DEAD, 3, dinoColor);
	else
		DrawSpriteRows(DINO_X, footY - 2, legFrame ? DINO_RUN1 : DINO_RUN0, 3, dinoColor);
}

void DinoContent::DrawObstacle(const Obstacle& o)
{
	int left = (int)(o.x + 0.5f);

	switch (o.type)
	{
	case _EObstacle::CACTUS_SMALL:
		DrawSpriteRows(left, o.topY, CACTUS_S, 2, (nightPhase == 1) ? DARKGREEN : GREEN);
		break;
	case _EObstacle::CACTUS_LARGE:
		DrawSpriteRows(left, o.topY, CACTUS_L, 3, (nightPhase == 1) ? DARKGREEN : GREEN);
		break;
	default:	// 익룡
		DrawSpriteRows(left, o.topY, wingFrame ? PTERO_B : PTERO_A, 2, ForeColor());
		break;
	}
}

void DinoContent::DrawGround()
{
	int col = ForeColor();

	for (int x = 1; x <= 38; x++)
		SCREEN->OnDrawColor(x * 2, GROUND_Y, "■", col);

	// 질감(점) ? 스크롤 오프셋 반영
	int off = (int)groundScrollX;
	for (int x = 1; x <= 38; x++)
		if (((x + off) % 5) == 0)
			SCREEN->OnDrawColor(x * 2, GROUND_Y + 1, "..", DARKGRAY);
}

void DinoContent::DrawBackground()
{
	// 구름
	for (size_t i = 0; i < clouds.size(); i++)
	{
		int cx = (int)(clouds[i].x + 0.5f);
		SCREEN->OnDrawColor(cx * 2, clouds[i].y, "■", DARKGRAY);
		SCREEN->OnDrawColor((cx + 1) * 2, clouds[i].y, "■", DARKGRAY);
		SCREEN->OnDrawColor((cx + 2) * 2, clouds[i].y, "■", DARKGRAY);
	}

	// 밤: 별 + 달
	if (nightPhase == 1)
	{
		SCREEN->OnDrawColor(24, 3, "*", WHITE);
		SCREEN->OnDrawColor(40, 4, "*", WHITE);
		SCREEN->OnDrawColor(52, 3, "*", WHITE);
		SCREEN->OnDrawColor(66, 4, "*", WHITE);
		SCREEN->OnDrawColor(70, 2, "●", YELLOW);	// 달
	}
}

void DinoContent::DrawScore()
{
	std::string hi = std::to_string(hiScore);
	while (hi.size() < 5) hi = "0" + hi;
	std::string sc = std::to_string(score);
	while (sc.size() < 5) sc = "0" + sc;

	SCREEN->OnDrawColor(46, 1, "HI", GRAY);
	SCREEN->OnDrawColor(50, 1, hi.c_str(), GRAY);

	bool showScore = (scoreBlink == 0) || ((scoreBlink / 3) % 2 == 0);
	if (showScore)
		SCREEN->OnDrawColor(60, 1, sc.c_str(), WHITE);
}
