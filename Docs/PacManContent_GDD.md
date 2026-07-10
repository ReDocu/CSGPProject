# PacManContent — Game Design Document (Console Pac-Man)

> CSGP(Console Game Pack) 프레임워크 기반 **콘솔 팩맨(PAC-MAN)**. 미로 액션 / 점수 아케이드.
> `MazeContent`(격자·BFS)·`BattleCityContent`(타일맵·AI)·`GalagaContent`(오브젝트 관리)와 동일 패턴(`IGameContent` 상속, `x*2` 좌표, `GetTickTimer`, 색 상수)을 따른다.
> 함께 볼 문서: [`CURRICULUM.md`](CURRICULUM.md), [`FUTURE_GAMES.md`](FUTURE_GAMES.md), [`handover.md`](../handover.md), [`CLAUDE.md`](../CLAUDE.md).
> 이 문서는 **설계 명세**이며 아직 코드는 없다. 구현 순서는 §11 마일스톤 참고. ([`FUTURE_GAMES.md`](FUTURE_GAMES.md)의 Pac-Man 항목을 정식 GDD로 승격.)

---

## 0. 이 게임의 설계 의도 (커리큘럼: **8단계 — 성격별 AI + 유한상태기계**)

커리큘럼 7종([`CURRICULUM.md`](CURRICULUM.md)) 이후의 **8단계** 후보. 이 게임의 배움 주제는 딱 하나 — **적 AI 설계**다.

- **성격별 타게팅 AI** — 고스트 4마리가 각자 **다른 목표 좌표**를 계산한다(Blinky 직접추적 / Pinky 앞질러 / Inky 협공 / Clyde 변덕). "AI = 목표 선정 + 경로 선택"을 손으로 만든다.
- **유한상태기계(FSM)** — 고스트 상태 `HOUSE → SCATTER ↔ CHASE → FRIGHTENED → EATEN` 를 시간/이벤트로 전이. 전역 모드 스케줄(scatter/chase 교대)이 개별 고스트를 몬다.
- **격자 위 경로 선택** — 교차로에서만 방향 결정 + **역방향 금지**. [`MazeContent`](MazeContent_GDD.md)의 **BFS를 재사용**해 "목표까지 다음 한 칸"을 계산할 수 있다(MVP는 고전 greedy).
- (보너스) **데이터 주도 맵 로딩** — 미로를 코드가 아니라 문자 배열로 authored → 파싱. [`Sokoban`](FUTURE_GAMES.md) 레벨 로딩과 연결되는 실습.

> 대조: Battle City의 적 AI는 "랜덤 + 기지 편향"이었다. 팩맨은 **정교한 타게팅 + 상태 전이**로 AI를 한 단계 끌어올린다.

---

## 1. 개요

| 항목 | 내용 |
|------|------|
| 제목 | PAC-MAN (CSGP 콘텐츠) |
| 장르 | 미로 액션 / 점수 아케이드 (실시간) |
| 플랫폼 | Windows 콘솔 (Win32 API) |
| 플레이 인원 | 1인 |
| 씬 클래스 | `PacManContent : public IGameContent` |
| 씬 enum | `_ECONTENT::PACMAN` (framework.h에 추가) |
| 목표 | 팩맨을 조작해 미로의 **모든 펠릿(`·`)을 획득** → 스테이지 클리어. 고스트에 닿으면 생명 감소, 생명 0이면 게임 오버 |

핵심 재미: 고스트의 추격을 읽고 파워펠릿으로 반격하는 심리전. 엔지니어링 목표는 **성격별 AI + FSM**을 명확히 구현하는 것.

---

## 2. 핵심 게임 루프

```
입력(방향 버퍼링) → 팩맨 이동(교차로 회전) → 펠릿/파워 획득
   → 고스트 모드 갱신(scatter/chase/frightened) → 고스트 타게팅·이동
   → 충돌 검사(팩맨↔고스트) → 점수·생명 → 클리어 판정 → 화면 출력 → 다음 프레임
```

프레임워크 고정 루프(~60fps) 위에서 **이동은 각자의 틱**(`GetTickTimer`)으로, 파워 지속/모드 스케줄/애니는 프레임 카운터 또는 틱으로 제어한다.

---

## 3. 화면 구성 & 좌표 매핑

논리 해상도 `GAME_SIZE 40×25`. 렌더 시 x는 항상 `×2`(전각 1칸=콘솔 2칸). `MainContent`가 최외곽 테두리를 그리므로 내부 x 1~38, y 1~23 사용. 미로는 **타일 격자**(타일 1개 = 글리프 1개).

```
 ┌────────────────────────────────────────────────┐ y=0 (MainContent 테두리)
 │ SCORE 001250   HI 014250   STAGE 3   ♥♥        │ y=1  상단 HUD
 │        ■■■■■■■■■■■■■■■■■■■           │ y=2  ← 미로 (steel/pellet)
 │        ■·······■·······■           │        (FIELD_Y0=2)
 │        ■·■■·■■·■·■■·■■·■           │
 │        ■o·············o■           │  o = 파워펠릿
 │        ■·■■·■·███·■·■■·■           │  ███ = 고스트 하우스
 │        ■······Ω Ω Ω Ω······■        │
 │        ■·■■·■·█-█·■·■■·■           │  - = 하우스 게이트
 │        ·····      ▶(P)     ·····     │  좌우 끝 = 터널(워프)
 │        ■o·············o■           │ y=22 (미로 하단)
 │──────────────────────────────────── │        구분선
 │  WASD 이동   P 정지   ESC 뒤로            │ y=23  조작 안내
 └────────────────────────────────────────────────┘ y=24
```

### 배치 상수 (논리좌표)
| 상수 | 값(예) | 의미 |
|------|----|------|
| `MAZE_W` / `MAZE_H` | ≤ 38 / ≤ 21 | 미로 타일 수 (authored 데이터가 결정). 예: 19 × 21 |
| `FIELD_X0` / `FIELD_Y0` | 중앙정렬 / 2 | 미로 타일 (0,0)의 논리 좌표. `FIELD_X0 = (40-MAZE_W)/2` |
| `HUD_Y` | 1 | 상단 정보줄 |
| `CONTROL_Y` | 23 | 조작 안내 |

- 타일 `(tx,ty)` → 논리 `(FIELD_X0+tx, FIELD_Y0+ty)` → **콘솔 출력 `((FIELD_X0+tx)*2, FIELD_Y0+ty)`**.
- 팩맨·고스트는 **정수 타일 좌표**로 이동(그리드 스냅). 이동 애니는 틱 단위. 로직·충돌은 타일 좌표 일치.
- **터널 워프**: 좌우 끝의 통로 행은 반대편으로 순환(x 랩).

---

## 4. 조작 (InputManager)

| 페이즈 | 키 | 동작 | 입력 방식 |
|--------|----|------|-----------|
| 인게임 | `W / A / S / D` (+ 방향키 보조) | 상/좌/하/우 **원하는 방향 예약**(교차로에서 반영) | `OnKeyStay` |
| 인게임 | `P` | 일시정지 토글 | `OnKeyDown` |
| 인게임 | `ESC` | 타이틀로 | `OnKeyDown` |
| 타이틀 | `SPACE / Enter` | 게임 시작 | `OnKeyDown` |
| 타이틀 | `ESC` | 프로그램 종료(main이 처리) | — |

> **방향 버퍼링(팩맨 손맛의 핵심)**: 입력은 즉시 `wantDir`에 저장하고, 이동 틱에 그 방향이 벽이 아니면 회전한다. 코너를 미리 눌러도 부드럽게 꺾인다.
> **ESC 규칙**: 프레임워크 관례(인게임 ESC→타이틀, 타이틀 ESC→종료). `main.cpp`가 `OnUpdate` 뒤에서 ESC 검사([handover.md](../handover.md) §8). 주신 스펙의 "ESC=종료"는 이 관례로 실현.

---

## 5. 엔티티 / 글리프 정의 (CP949 왕복 검증 완료)

| 요소 | 글리프 | 색(framework.h) | 비고 |
|------|--------|------------------|------|
| 팩맨(입 닫힘) | `●`(U+25CF) | `YELLOW`(14) | 애니: 닫힘 ↔ 열림 토글 |
| 팩맨(입 열림) | `▶ ◀ ▲ ▼` | `YELLOW`(14) | 진행 방향으로 입 벌림 |
| 펠릿 | `·`(U+00B7) | `WHITE`(15) | 미로 타일, 먹으면 사라짐 |
| 파워 펠릿 | `●`(U+25CF) | `WHITE`(15) | 깜빡임(크게), 파워모드 시작 |
| 고스트(일반) | `Ω`(U+03A9) | 성격별(아래) | Blinky/Pinky/Inky/Clyde |
| 고스트(프라이트) | `Ω` | `DARKBLUE`(1) | 종료 직전 `WHITE` 깜빡 |
| 고스트(먹힘·눈) | `◎`(U+25CE) | `WHITE`(15) | 하우스로 복귀 |
| 벽 | `■`(U+25A0) | `BLUE`(9) | 이동 불가 |
| 하우스 게이트 | `-` / `≡`(U+2261) | `GRAY`(7) | 고스트만 통과 |
| 생명 | `♥`(U+2665) | `RED`(12) | HUD |

**고스트 성격별 색**: Blinky `RED`(12) · Pinky `PURPLE`(13) · Inky `SKYBLUE`(11) · Clyde `DARKYELLOW`(6).

> 검증 완료: `● ▶◀▲▼ · Ω ◎ ■ ♥ ≡`(모두 CP949 round-trip True). 단각 `-`는 ASCII. (§12)

---

## 6. 게임 시스템

### 6.1 미로 (데이터 주도 로딩)
- 미로는 **문자 배열로 authored** → `LoadMaze()` 가 `_ETile tile[MAZE_H][MAZE_W]` 로 파싱.
- 타일 심볼: `#`=WALL, `.`=PELLET, `o`=POWER, ` `(공백)=EMPTY, `-`=DOOR(게이트), `P`=팩맨 시작, `G`=고스트 시작(하우스).
- **좌우 대칭** 권장(팩맨 특유의 미로). 스테이지별 다른 미로 데이터 or 동일 미로 + 난이도 파라미터.
- 펠릿/파워는 **타일에 저장**하고 `pelletCount` 로 남은 수 추적(먹으면 타일→EMPTY, 카운트--). `pelletCount==0` → 스테이지 클리어.

### 6.2 팩맨 (Position / Direction / Life / Score / State)
- 이동: `GetTickTimer(pacMove)` 마다 `dir` 방향 1타일(벽이면 정지). 매 프레임 `wantDir` 갱신 → 틱에서 회전 가능하면 반영(§4 버퍼링).
- 터널 워프: 좌우 끝 통로에서 반대편으로.
- 획득: PELLET → +10·카운트--, POWER → +50·**파워모드 시작**(§6.5).
- 피격: NORMAL에서 고스트와 같은 타일 → `life--`, DYING 연출 후 리스폰. `life<=0` → GAME OVER.

### 6.3 ⭐ 고스트 & AI (성격별 타게팅) — 이 게임의 핵심
고스트 4마리, 각자 **목표 타일(targetX,targetY)** 을 다르게 계산하고, **교차로에서 목표에 가장 가까워지는 방향**(역방향 제외)을 greedy 선택한다(고전 팩맨 방식). 직선 통로는 계속 진행.

| 고스트 | 색 | CHASE 목표 | SCATTER 목표(코너) |
|--------|----|-----------|---------------------|
| **Blinky** | RED | 팩맨 타일(직접 추적) | 우상단 |
| **Pinky** | PURPLE | 팩맨 진행방향 **4칸 앞** | 좌상단 |
| **Inky** | SKYBLUE | 팩맨 2칸 앞 기준 Blinky 반사 벡터(협공) | 우하단 |
| **Clyde** | DARKYELLOW | 팩맨과 **8타일 밖이면 추적, 안이면** 자기 코너 | 좌하단 |

- **교차로 판정**: 현재 타일에서 진행 가능한(벽 아님) 이웃이 3개 이상 or 방향 전환 필요 시. 역방향은 **모드 전환 순간에만** 허용.
- **BFS 재사용(폴리시)**: greedy 대신 [`MazeContent`](MazeContent_GDD.md)의 BFS로 목표까지 최단 다음 칸 계산 → 더 똑똑한 AI. MVP는 greedy(원작 충실).

### 6.4 고스트 FSM (Spawn/Scatter/Chase/Frightened/Eaten)
```
HOUSE ──(대기시간 경과)──▶ SCATTER ◀──(전역 모드 스케줄)──▶ CHASE
   ▲                          │  파워펠릿                    │
   │                          ▼                              │
 (하우스 도착)             FRIGHTENED ──(팩맨에 먹힘)──▶ EATEN(눈) ──▶ HOUSE
```
- **전역 모드 스케줄**: `SCATTER↔CHASE` 를 시간표로 교대(예 7s scatter → 20s chase, 스테이지 오를수록 chase↑). `modeTimer` 로 관리.
- **FRIGHTENED**: 파워펠릿 획득 시 전체 진입(이동 느려지고 교차로에서 **랜덤**), 종료 직전 흰색 깜빡. 먹히면 EATEN.
- **EATEN**: 눈(`◎`)만 남아 **빠르게** 하우스 게이트로 복귀(BFS/greedy) → HOUSE → 재출발.
- 모드 전환 시 고스트는 **진행 방향을 반전**(원작 규칙).

### 6.5 파워 모드
- 지속 `POWER_SEC`(기본 8초, 스테이지↑ 시 감소) — `powerTimer`(프레임) 카운트다운.
- 효과: 전체 고스트 FRIGHTENED, 팩맨이 고스트를 먹을 수 있음.
- **연쇄 점수**: 한 파워모드 동안 먹은 고스트 순서대로 `200 → 400 → 800 → 1600`(먹을 때마다 배가). 파워 재획득 시 연쇄 리셋.

### 6.6 충돌 판정
- **팩맨 × 펠릿/파워**: 팩맨 타일의 PELLET/POWER 처리.
- **팩맨 × 고스트**(같은 타일):
  - 고스트 NORMAL(SCATTER/CHASE) → 팩맨 사망(§6.2).
  - 고스트 FRIGHTENED → 고스트 EATEN + 연쇄 점수.
  - 고스트 EATEN(눈) → 무시.
- **고스트 × 벽**: 통행 불가(경로 선택에서 제외). 게이트는 고스트만 통과.
- 판정은 타일 정수 좌표 일치.

### 6.7 점수 · 생명 · 스테이지
- 점수: 펠릿 `10` / 파워펠릿 `50` / 고스트 연쇄 `200·400·800·1600` / **스테이지 클리어 `+1000`**.
- 생명: 시작 `3`, HUD `♥`×남은 수. `0`이면 GAME OVER. HI 스코어 세션 유지(파일 저장은 §13).
- 스테이지: 오를수록 **고스트 속도↑ · CHASE 비중↑ · 파워시간↓**. 미로는 동일 or 교체.

### 6.8 애니메이션
- 팩맨: `●`(닫힘) ↔ 방향 글리프(열림) 토글(`GetTickTimer(0.12f)`). 사망: 짧은 연출 후 리스폰.
- 고스트: FRIGHTENED 파랑 + 종료 직전 흰 깜빡, EATEN 눈만.
- 파워펠릿: 깜빡임.
- (선택) `Beep` 효과음 — 프레임 끊김 주의(Dino/Galaga와 동일, 아주 짧게 or 생략).

---

## 7. 상태 머신

`IGameContent`의 페이즈(`TITLE/INGAME`)는 공용, 인게임 세부 상태는 내부 enum. 주신 스펙의 상태(Idle/Ready/Playing/PowerMode/StageClear/GameOver)를 매핑:

```
IGameContent::_EPhase
 ├─ TITLE  → OnTitleUpdate/Render  (PAC-MAN 로고 + PRESS SPACE)
 └─ INGAME → OnInGameUpdate/Render
       └─ PacManContent::_EGameState
            ├─ READY      : "READY!" 짧은 대기(팩맨·고스트 배치 표시)
            ├─ PLAYING    : 정상 플레이 (파워모드는 powerTimer>0 서브플래그)
            ├─ PAUSE      : 일시정지(P)
            ├─ DYING      : 팩맨 사망 연출 → 리스폰 or GAMEOVER
            ├─ STAGECLEAR : "STAGE CLEAR +1000" → 다음 스테이지
            └─ GAMEOVER   : 결과 + 재시작(SPACE/Enter)
```
- 고스트 상태(`_EGhostState`)는 §6.4의 FSM로 **각 고스트마다** 별도 관리.
- 재시작/다음 스테이지: 미로 재로딩 + 팩맨·고스트 리셋(값 리셋).

---

## 8. 프레임워크 통합

### 8.1 추가/수정 파일
| 파일 | 작업 |
|------|------|
| `framework.h` | `enum class _ECONTENT`에 `PACMAN` 추가(끝에, 정수 순서 유지) |
| `Main/MainContent.cpp` | `SCENE->AddContent((int)_ECONTENT::PACMAN, new PacManContent());` 등록 |
| `Main/Content/PacManContent.h` | 신규 — `IGameContent` 상속 (ASCII 주석 권장) |
| `Main/Content/PacManContent.cpp` | 신규 — 전각 글리프 포함 → **CP949 저장** |
| `CSGP.vcxproj` / `.filters` | 신규 두 파일 등록(UTF-8) |

### 8.2 스펙의 "개발 구조" → CSGP 매핑
게임은 **하나의 `IContent` 씬**. 공용 매니저는 프레임워크가 제공, 게임 고유 모듈은 씬 내부 배열 + 메서드:

| 스펙 모듈 | CSGP 실현 |
|-----------|-----------|
| Input / Renderer | `INPUT` / `SCREEN` + `DrawXxx()` |
| Map(Tile[][]) | `_ETile tile[MAZE_H][MAZE_W]` + `LoadMaze()` |
| Player | `px/py/pdir/life/score/...` + `UpdatePlayer()` |
| **Ghost / AI** | **`Ghost ghosts[4]` + `UpdateGhosts()` + `ChooseGhostDir()`** |
| Collision | `HandleCollisions()` |
| Stage / Difficulty | `stage` + `StartStage()` |
| Score / HUD | `score/hiScore` + `DrawHUD()` |
| Sound / Save | `Beep()`(선택) / 하이스코어 파일(§13) |

> 고스트는 **고정 4마리 배열**이라 오브젝트 풀이 불필요(Galaga/BC와 달리 이 게임의 축은 **AI**). 원한다면 [`Pool.h`](../Main/Content/Pool.h)로 통일 가능하나 필수는 아니다.

### 8.3 클래스 설계 (헤더 스케치)
```cpp
class PacManContent : public IGameContent {
private:
    static const int MAZE_W = 19, MAZE_H = 21;   // authored 데이터에 맞춤
    int FIELD_X0, FIELD_Y0;                        // 중앙정렬 오프셋

    enum class _EGameState { READY, PLAYING, PAUSE, DYING, STAGECLEAR, GAMEOVER };
    enum class _ETile      { EMPTY, WALL, PELLET, POWER, DOOR };
    enum class _EDir       { UP, RIGHT, DOWN, LEFT, NONE };
    enum class _EGhost     { BLINKY, PINKY, INKY, CLYDE };
    enum class _EGhostState{ HOUSE, SCATTER, CHASE, FRIGHTENED, EATEN };

    struct Ghost {
        _EGhost   who;   _EGhostState state;
        int       x, y;  _EDir dir;
        int       homeX, homeY;     // scatter 코너
        int       moveT;            // 개별 이동 타이머(프레임)
    };

    _ETile tile[MAZE_H][MAZE_W];
    int    pelletCount;

    // player
    int   px, py; _EDir pdir, wantDir;
    int   life, score, hiScore, stage;
    int   powerTimer;     // >0 이면 파워모드
    int   eatChain;       // 연쇄 고스트 점수 인덱스

    // ghosts / mode
    Ghost ghosts[4];
    int   modeTimer;      // scatter/chase 전역 스케줄
    bool  chaseMode;
    int   frameCount;

    _EGameState gameState;

public:
    virtual void OnInit();  virtual void OnRelease();
    virtual void OnTitleUpdate();   virtual void OnTitleRender();
    virtual void OnInGameUpdate();  virtual void OnInGameRender();

private:
    void StartGame();       void StartStage(int stage);
    void LoadMaze(int stage);                 // 문자 배열 → tile[][]
    void UpdatePlayer();
    void UpdateGhosts();                       // 모드 갱신 + 이동
    void ChooseGhostDir(Ghost& g);             // 성격별 타게팅 + greedy
    void GhostTarget(const Ghost& g, int& tx, int& ty);
    void HandleCollisions();
    void KillPlayer();      void EatGhost(Ghost& g);

    bool Passable(int tx, int ty, bool isGhost) const;
    void DirDelta(_EDir d, int& dx, int& dy) const;

    int  TX(int tx) const { return (FIELD_X0 + tx) * 2; }
    int  TY(int ty) const { return FIELD_Y0 + ty; }
    void DrawMaze();  void DrawPlayer();  void DrawGhosts();  void DrawHUD();
};
```

### 8.4 타이머 사용 요약
| 용도 | 방식 | 키(초, 예) |
|------|------|--------|
| 팩맨 이동 | `GetTickTimer(pacMove)` | 스테이지별 0.14→0.10 |
| 고스트 이동(일반) | `GetTickTimer(ghostMove)` | 0.16→0.10 |
| 고스트 이동(프라이트/먹힘) | 별도 키 | 0.22 / 0.07 |
| 팩맨 입 애니 | `GetTickTimer(0.12f)` | 0.12 |
| 파워 지속 · 모드 스케줄 | 프레임 카운트 | — |

---

## 9. 렌더링 계획 (OnInGameRender)

1. **미로**: `tile[][]` — 벽 `■`(BLUE), 펠릿 `·`, 파워 `●`(깜빡), 게이트 `-`.
2. **고스트**: 상태별 색/글리프(NORMAL 성격색 `Ω` / FRIGHTENED `Ω`DARKBLUE / EATEN `◎`).
3. **팩맨**: `●`↔방향 글리프 토글, YELLOW.
4. **HUD**: 상단 y=1 `SCORE / HI / STAGE / ♥×n`, 파워모드 시 `POWER n`.
5. **오버레이**: `READY!`, `PAUSE`, `STAGE CLEAR +1000`, `GAME OVER`.

> 성능: 미로 타일 순회(빈칸 skip) + 고스트 4 + 팩맨 → 셀 호출 적음. 매 프레임 전면 렌더로 충분(Battle City와 동일).

---

## 10. 핵심 데이터 요약

```
tile[MAZE_H][MAZE_W] (_ETile)   : 미로 (벽/펠릿/파워/게이트) + pelletCount
px,py,pdir,wantDir              : 팩맨 (방향 버퍼링)
life,score,hiScore,stage        : 진행
powerTimer,eatChain             : 파워모드 / 연쇄 점수
ghosts[4] (who,state,x,y,dir…)  : 고스트 + 개별 FSM
modeTimer,chaseMode             : 전역 scatter/chase 스케줄
gameState                       : READY/PLAYING/PAUSE/DYING/STAGECLEAR/GAMEOVER
```

---

## 11. 구현 마일스톤 (주신 §25 개발 우선순위 → Phase 매핑)

| Phase | 목표 | 산출물 |
|:---:|------|--------|
| **1** | **맵 & 팩맨** | PacManContent 등록(`_ECONTENT::PACMAN`), `LoadMaze`(문자→타일)·미로 렌더, 팩맨 이동(방향 버퍼링)·벽 충돌, **펠릿 먹기·점수**, 스테이지 클리어 판정 |
| **2** | **고스트 기본** | 고스트 스폰·이동(1마리 Blinky CHASE), 팩맨↔고스트 충돌·사망·생명·리스폰, READY/GAME OVER |
| **3** | **AI & 파워** | 4마리 **성격별 타게팅** + `SCATTER/CHASE/FRIGHTENED/EATEN` FSM + 전역 모드 스케줄, 파워펠릿·고스트 먹기·연쇄 점수 |
| **4** | **완성** | 스테이지 진행·난이도(속도↑·파워시간↓·chase↑), 애니(입/프라이트/눈) 다듬기, 하이스코어, (선택) **BFS AI**·사운드·저장 |

**권장 순서**: Phase 1~3이 "미로에서 먹고 쫓기는 최소 팩맨". **AI/FSM은 Phase 3의 핵심** — 여기에 학습 가치 집중. Phase마다 빌드(`MSBuild CSGP.sln /p:Configuration=Debug /p:Platform=x64`)로 검증.

### MVP 목표(스펙)
팩맨 4방향 이동 · 펠릿 먹기 · 벽 충돌 · 고스트 AI · 파워모드 · 점수 · 생명 · 스테이지 진행 · 게임 오버 · 콘솔 실시간. — Phase 1~4로 충족.

---

## 12. 인코딩 · 주의사항 (CLAUDE.md / handover.md 연동)

- 신규 `.cpp`에 전각 글리프·한글 문자열 → **반드시 CP949 저장**(UTF-8 작성 → PowerShell 재인코딩 + 왕복 검증). 헤더는 ASCII 권장. ([CLAUDE.md](../CLAUDE.md) 인코딩 규칙)
- **검증된 글리프**(구현 시 그대로 사용): `●`(팩맨/파워) · `▶◀▲▼`(팩맨 방향) · `·`(U+00B7 펠릿) · `Ω`(U+03A9 고스트) · `◎`(U+25CE 눈) · `■`(벽) · `♥`(생명) · `≡`(게이트) — 전부 CP949 round-trip 확인. 단각 `-`는 ASCII.
- **전각/단각 정렬**: 전각은 `logicalX*2`에 2칸, 로직·충돌은 **타일 정수 좌표**로만. (§3)
- `framework.h`의 `_ECONTENT`에 `PACMAN`을 **끝에 추가**(정수 순서 유지).
- **`Windows.h` 이름 충돌 주의**: `RED/GREEN/BLUE`는 색상수, `IN/OUT/NEAR/FAR/small` 등. enum/구조체는 `_E…`/`Ghost`처럼 안전하게. (Battle City에서 `ACCEL` 충돌 사례 참고)
- **ESC 규칙**: 인게임 ESC→타이틀, 타이틀 ESC→종료([handover.md](../handover.md) §8).

---

## 13. 향후 확장 (Out of Scope for MVP)

- **BFS 기반 고스트 AI** — greedy 대신 [`MazeContent`](MazeContent_GDD.md) 경로탐색 재사용(벽 우회·정밀 추적).
- **랜덤 미로 생성** — Maze의 생성 알고리즘으로 스테이지마다 새 미로.
- **과일 보너스 · 다양한 고스트 · 아이템**, 컷신, 무한/시간제한 모드, 보스 고스트.
- **저장 데이터**([CURRICULUM.md](CURRICULUM.md) §12) — 하이스코어·최고 스테이지·총 펠릿/고스트 처치 수 파일 저장.
- **맵 에디터** — authored 미로를 외부 파일로(데이터 주도 심화, [`Sokoban`](FUTURE_GAMES.md) 레벨 로딩과 연계).

---

*본 GDD는 CSGP 프레임워크(v0.2) 기준. 커리큘럼 8단계(AI·FSM) 후보로, 착수 시 Phase 1(맵+팩맨)부터 진행 권장. 원본 아이디어 백업: [`FUTURE_GAMES.md`](FUTURE_GAMES.md) §1.*
