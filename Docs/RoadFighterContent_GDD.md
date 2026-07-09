# RoadFighterContent — Game Design Document

> CSGP(Console Game Pack) 프레임워크 기반 **Road Fighter(콘솔 레이싱)**.
> 사용자 제공 초안(24개 섹션)을 프레임워크에 맞춰 구현 가능한 명세로 재구성한 문서.
> `SnakeContent`/`TetrisContent`/`DinoContent`와 동일 패턴(`IGameContent` 상속, `x*2` 좌표, `GetTickTimer`, 색 상수)을 따른다.
> 함께 볼 문서: [`handover.md`](../handover.md), [`CLAUDE.md`](../CLAUDE.md), [`DinoContent_GDD.md`](DinoContent_GDD.md).
> 이 문서는 **설계 명세**이며 아직 코드는 없다. 구현 순서는 §11 마일스톤 참고.

---

## 1. 개요

| 항목 | 내용 |
|------|------|
| 제목 | ROAD FIGHTER (CSGP 콘텐츠) |
| 장르 | 2D 콘솔 레이싱 / 회피 액션 |
| 플랫폼 | **Windows 콘솔** (Win32 API) — 아래 적응 참고 |
| 플레이 인원 | 1인 |
| 씬 클래스 | `RoadFighterContent : public IGameContent` |
| 씬 enum | `_ECONTENT::ROADFIGHTER` (framework.h에 추가) |
| 목표 | 5차선 도로에서 적 차량·장애물을 피하고 연료를 관리하며 최대한 멀리·빠르게 달려 고득점 |

핵심 재미: **가속/감속 판단 + 차선 회피**의 반사신경 긴장감. 속도가 오를수록 점수는 빨라지지만 연료 소모·차량 밀도가 늘어 위험도 커진다.

### 초안 → 콘솔/프레임워크 적응 (변경 명시)
| 초안 | 이 GDD | 사유 |
|------|--------|------|
| 크로스플랫폼(Win/Linux/macOS) | **Windows 전용** | CSGP는 Win32 콘솔 API(`CreateConsoleScreenBuffer`, `GetAsyncKeyState`) 직접 사용 |
| 차량 위치 자유(화면 목업) + "차선 단위 이동"(본문) | **차선 단위(5차선) 이동** | 초안 본문 명시(§10, §24)를 채택 → 충돌 판정 단순·안정. 자유 이동은 §15 확장 |
| HUD 상/하단 분할 | **상단 압축 HUD + 도로** (또는 우측 패널, §3 노트) | 세로 런웨이 확보 절충 |
| 벽 `│`(반각) | **`■` 셀(전각)** | `x*2` 셀 그리드 정렬 |
| 도로 폭/차선 수 픽셀값 | 5차선, 논리 좌표로 재정의(§3) | 40×25 해상도에 맞춤 |
| 난이도 시간축(1·2·3·5분) | **압축(0/30/60/90/120s, 튜닝)** | 한 세션에 후반 스테이지 도달 가능하도록 |

---

## 2. 핵심 게임 루프 (초안 §23 반영)

```
타이틀 → SPACE 시작 → [매 프레임]
   입력(차선 이동 / 가속·감속) → 도로 스크롤 → 적/아이템 스폰·이동
   → 충돌 검사 → 점수 계산 → 연료 감소 → 시간 경과로 난이도↑
   → 연료 0 또는 목숨 0 이면 GAME OVER(결과) → SPACE 재시작
```

프레임워크 고정 루프(`OnUpdate → OnRender`, ~60fps) 위에서 **스크롤·물리는 프레임당 누적 스텝**, 스폰·애니메이션·타이밍은 `TIMER->GetTickTimer()`/`GetContentTime()`로 제어.

---

## 3. 화면 레이아웃 & 좌표 매핑

논리 해상도 `GAME_SIZE 40×25`. 렌더 시 x는 항상 `×2`(전각 1칸=콘솔 2칸). `MainContent`가 최외곽 테두리를 그리므로 내부 x 1~38, y 1~23 사용.

```
 논리좌표 기준 (초안 §4 목업을 콘솔에 맞춘 배치)
 x:0                                              38
 ┌────────────────────────────────────────────────┐ y=0 (MainContent 테두리)
 │ SCORE 00012300    STAGE 2      HIGH 00053400    │ y=1  상단 HUD
 │ FUEL  ███████░░░  SPEED 180km/h                 │ y=2
 ├────────────────────────────────────────────────┤ y=3  구분선
 │█ L0   L1   L2   L3   L4 █                       │ y=4  도로 시작(벽 + 5차선)
 │█  ▲          ■          █                       │
 │█       ■           ◆    █    (스크롤: 위→아래)   │
 │█            ■■          █                       │
 │█   ■■             ★     █                       │
 │█        ■■■■            █    ■■■■ = 트럭         │
 │█             ■          █                       │
 │█   ▟▛  ← 플레이어(하단 고정 근처)      █          │ y=21 플레이어 존
 │████████████████████████████████████████████████│ y=22 도로 끝
 │ LIFE ♥ ♥ ♥                                      │ y=23 하단 HUD
 └────────────────────────────────────────────────┘ y=24
```

### 배치 상수 (논리좌표)
| 상수 | 값 | 의미 |
|------|----|------|
| `ROAD_TOP` | 4 | 도로 상단 행 |
| `ROAD_BOTTOM` | 22 | 도로 하단 행(벽 바닥) |
| `ROAD_LEFT` | 1 | 좌측 벽 x |
| `ROAD_RIGHT` | 38 | 우측 벽 x |
| `LANE_COUNT` | 5 | 차선 수 |
| `PLAYER_Y` | 20 | 플레이어 발(하단 고정) y |
- 차선 폭 `laneW = (ROAD_RIGHT - ROAD_LEFT - 1) / LANE_COUNT` (≈7 셀). 차선 i 중심 x ≈ `ROAD_LEFT + 1 + i*laneW + laneW/2`. 구현 시 `LaneCenterX(i)` 헬퍼로 계산.
- 플레이어는 **하단 근처(y≈20)에 고정**, 적/아이템/장애물이 위에서 아래로 스크롤되어 다가온다.
- 셀 출력은 `OnDrawColor(logicalX*2, y, ...)`, HUD 텍스트는 콘솔 x 직접 지정.

> **레이아웃 노트**: 위 배치는 초안의 상/하단 HUD를 따른다(도로 런웨이 ≈17행). 반응 시간을 더 원하면 HUD를 **우측 패널**로 옮겨 도로를 y1~23(≈22행)까지 넓히는 변형이 가능하다(Snake/Tetris 방식). 구현 시 선택.

---

## 4. 조작 (InputManager, 초안 §5)

| 키 | 동작 | 입력 방식 |
|----|------|-----------|
| `←` / `→` | 플레이어를 왼쪽/오른쪽 **차선으로 이동** | `OnKeyDown`(1탭=1차선) + DAS 반복(선택) |
| `↑` | 가속(속도↑) | `OnKeyStay` |
| `↓` | 감속(속도↓) | `OnKeyStay` |
| `SPACE` | 시작 / 게임오버 후 재시작 | `OnKeyDown` |
| `P` | 일시정지 토글 | `OnKeyDown` |
| `ESC` | 타이틀로(인게임) / 종료(타이틀) | `OnKeyDown` — Dino·Tetris와 동일 규칙 |

> 차선 이동은 **엣지 트리거**로 한 탭당 한 차선(벽 넘어가면 클램프 → 초안 §7 "벽을 넘을 수 없다"). 가속/감속은 누르는 동안 연속. ESC 뒤로가기(인게임→타이틀, 타이틀→종료)는 기존 `main.cpp` 구조를 그대로 활용([handover.md](../handover.md) §9).

---

## 5. 엔티티 정의 (초안 §6, §9, §13)

모든 차량은 전각 블록 `■`로 표현(초안 §24). 아이템/장애물만 `◆ ★ ▲` 사용. 색은 가독성을 위해 종류별로 구분.

### 5.1 플레이어 (초안 §8)
- 스프라이트(3×3): ` ■ / ■■■ / ■ `(십자형). 히트박스 3×3.
- 속성: 차선 인덱스(0~4), 속도, 연료, 목숨, 점수, 무적 타이머.
- 초기값: **Life 3, Fuel 100%, Speed 80km/h, Score 0**.
- 색: `SKYBLUE`(11).

### 5.2 적 차량 (초안 §9) — 위→아래 스크롤
| 종류 | 스프라이트 | 크기 | 특징 | 색 |
|------|-----------|------|------|----|
| 일반(NORMAL) | `■■ / ■■` | 2×2 | 도로 속도로 하강(추월 대상) | `RED`(12) |
| 고속(FAST) | `■■ / ■■` | 2×2 | 가끔 좌우 차선 변경(위빙) | `PURPLE`(13) |
| 트럭(TRUCK) | `■■■■ / ■■■■ / ■■■■` | 4×3 | 크고 느림, 차선 대부분 차지 | `DARKYELLOW`(6) |
- 스폰: 도로 상단 밖(`y = ROAD_TOP - height`)에서 **랜덤 차선**.

### 5.3 아이템 / 장애물 (초안 §12, §13)
| 종류 | 글리프 | 효과 | 색 |
|------|--------|------|----|
| 연료(FUEL) | `◆` | Fuel +25% | `GREEN`(10) |
| 보너스(BONUS) | `★` | Score +500 | `YELLOW`(14) |
| 장애물(OBSTACLE) | `▲` | 충돌 시 피해(적 차량과 동일) | `DARKYELLOW`(6) |
- 아이템/장애물도 도로 속도로 하강. 연료·보너스는 획득 시 제거.

### 5.4 벽 · 도로 (초안 §7, §10)
- 좌/우 벽: `ROAD_LEFT`, `ROAD_RIGHT` 열에 `■`(`DARKGRAY`).
- 차선 구분선: 차선 경계에 스크롤되는 점선(`·`/`■` `DARKGRAY`)으로 속도감 표현.
- 도로면은 비움(검정).

---

## 6. 게임 시스템

### 6.1 차선 이동 (플레이어)
- `←`/`→` `OnKeyDown` → `playerLane` ±1, `[0, LANE_COUNT-1]`로 클램프(벽).
- 렌더 x = `LaneCenterX(playerLane) - 1`(3폭 중앙 정렬).
- (선택) 부드러운 전환: 목표 차선으로 x를 보간. MVP는 즉시 스냅.

### 6.2 속도 & 스크롤 (초안 §2)
- `speed`(km/h 표시), 범위 `[SPEED_MIN=80, SPEED_MAX=300]`.
  - `↑` 유지 → `speed += ACCEL`; `↓` 유지 → `speed -= ACCEL`; 미입력 시 스테이지 기준 속도로 서서히 수렴.
- **스크롤 누적**: 매 프레임 `scrollAccum += speed * SCROLL_K`. `scrollAccum >= 1`이면 모든 적/아이템/장애물을 아래로 1셀 이동하고 `scrollAccum -= 1`, 거리·점수 가산.
- 초기 제안: `SCROLL_K ≈ 0.001`(speed 300 → ≈0.3셀/프레임, 17행을 ≈1초에 통과 / speed 80 → ≈0.08셀/프레임). **튜닝값**.

### 6.3 적 차량 스폰 & 이동 (초안 §9, §15)
- **스폰 타이머**: `GetTickTimer(spawnInterval)`; `spawnInterval`은 스테이지·속도에 따라 감소.
- 스폰 시 랜덤 차선 + 스테이지별 타입 확률:
  | 스테이지 | NORMAL | FAST | TRUCK |
  |----------|--------|------|-------|
  | 1 | 100% | – | – |
  | 2 | 70% | 30% | – |
  | 3 | 55% | 30% | 15% |
  | 4~5 | 40% | 35% | 25% |
- 겹침 방지: 같은 차선 최상단 차량과 최소 세로 간격 확보(점프 없는 게임이므로 회피 가능한 간격).
- FAST는 일정 주기마다 인접 빈 차선으로 1칸 이동(위빙).

### 6.4 충돌 (AABB, 초안 §11)
- 플레이어 히트박스 vs (적/트럭/장애물) AABB 겹침.
- 충돌 시: **목숨 -1, 속도 → SPEED_MIN, 무적 ≈1.5초(깜빡)**. 무적 중 충돌 무시.
- 벽은 차선 클램프로 대체(충돌 피해 없음).
- **목숨 0 → GAME OVER**.

### 6.5 연료 (초안 §12)
- `fuel` 0~100. 매 프레임 `fuel -= FUEL_DRAIN * (1 + speedFactor) * dt`.
  - 초기 제안: 저속 ≈2%/s, 고속 ≈5%/s(연료 아이템 없이 ≈40~50초). **튜닝값**.
- 연료(`◆`) 획득 → `fuel = min(100, fuel + 25)`.
- **연료 0 → GAME OVER**.

### 6.6 점수 (초안 §14)
| 요소 | 점수 |
|------|------|
| 주행(스크롤 1셀) | +1 |
| 고속 유지(speed > 임계) 1셀 | +2(추가) |
| 차량 회피(적이 하단 밖으로 무충돌 통과) | +20 |
| 보너스 아이템 `★` | +500 |
- HIGH SCORE는 세션 내 유지(파일 저장은 §15 확장).

### 6.7 스테이지 / 난이도 (초안 §15, §16, §20)
- `stage`는 `GetContentTime()` 기준(초기 제안, 튜닝):
  | 시간(s) | 0 | 30 | 60 | 90 | 120+ |
  |---------|---|----|----|----|----|
  | 스테이지 | 1 | 2 | 3 | 4 | 5 |
- 스테이지↑ → 스폰 간격 감소, 기준 속도 상승, 트럭 확률 증가, 연료 소모 증가(초안 §15).

---

## 7. 상태 머신 (초안 §18)

`IGameContent`의 페이즈(`TITLE/INGAME`)는 공용이라 수정하지 않고, 인게임 세부 상태는 내부 enum으로 관리(Dino/Tetris와 동일).

```
IGameContent::_EPhase
 ├─ TITLE   → OnTitleUpdate/Render  (ROAD FIGHTER 로고 + PRESS SPACE, 데모 도로)
 └─ INGAME  → OnInGameUpdate/Render
       └─ RoadFighterContent::_EGameState
            ├─ PLAYING   : 정상 주행
            ├─ PAUSE     : 일시정지 오버레이(P)
            └─ GAMEOVER  : 결과 화면(SCORE/DISTANCE/HIGH), SPACE 재시작
```
- 결과(RESULT) 화면은 GAMEOVER 상태 렌더로 통합(초안 §19).
- `ESC`(인게임) → 타이틀. 타이틀에서 `ESC` → `main.cpp`가 종료.

---

## 8. 프레임워크 통합

### 8.1 추가/수정 파일
| 파일 | 작업 |
|------|------|
| `framework.h` | `enum class _ECONTENT`에 `ROADFIGHTER` 추가(끝에) |
| `Main/MainContent.cpp` | `SCENE->AddContent((int)_ECONTENT::ROADFIGHTER, new RoadFighterContent());` 등록 |
| `Main/Content/RoadFighterContent.h` | 신규 — `IGameContent` 상속(ASCII 주석 권장) |
| `Main/Content/RoadFighterContent.cpp` | 신규 — 전각/한글 포함 → **CP949 저장** |
| `CSGP.vcxproj` / `.filters` | 신규 두 파일 등록(UTF-8, Edit 가능) |

### 8.2 클래스 설계 (헤더 스케치)
```cpp
class RoadFighterContent : public IGameContent {
private:
    static const int ROAD_TOP    = 4;
    static const int ROAD_BOTTOM = 22;
    static const int ROAD_LEFT   = 1;
    static const int ROAD_RIGHT  = 38;
    static const int LANE_COUNT  = 5;
    static const int PLAYER_Y    = 20;

    enum class _EGameState { PLAYING, PAUSE, GAMEOVER };
    enum class _EEntity    { ENEMY_NORMAL, ENEMY_FAST, TRUCK, FUEL, BONUS, OBSTACLE };

    struct Entity {
        _EEntity type;
        int   lane;     // 소속 차선(위빙/스폰용)
        float y;        // 세로 위치(float, 스크롤)
        int   w, h;     // 히트박스
    };

    // 플레이어
    int   playerLane;
    float speed;        // km/h 표시값
    int   fuel;         // 0~100 (float로 관리 후 표시 반올림도 가능)
    float fuelF;
    int   life;
    bool  invincible;   // 피격 무적
    int   invTimer;     // 무적 남은 프레임

    std::vector<Entity> entities;

    float scrollAccum;
    int   score, hiScore;
    int   distance;     // m
    int   stage;

    _EGameState gameState;

public:
    virtual void OnInit();          virtual void OnRelease();
    virtual void OnTitleUpdate();   virtual void OnTitleRender();
    virtual void OnInGameUpdate();  virtual void OnInGameRender();

private:
    void StartGame();
    void UpdatePlayer();     // 차선 이동 / 가속·감속 / 무적
    void UpdateWorld();      // 스크롤 / 스폰 / 제거
    void UpdateFuel();
    void UpdateStage();
    void SpawnEntity();
    bool CheckCollision();   // 플레이어 vs 엔티티
    int  LaneCenterX(int lane) const;

    void DrawRoad();
    void DrawEntities();
    void DrawPlayer();
    void DrawHUD();
};
```
> **메모리 모델**: 모든 오브젝트를 값 타입 `std::vector<Entity>`로 관리(raw 포인터·수동 delete 회피). `OnRelease`는 `clear` 정도로 최소화.

### 8.3 타이머 사용 요약
| 용도 | 방식 |
|------|------|
| 스크롤/물리 | 매 프레임 누적 스텝(`scrollAccum`) |
| 적/아이템 스폰 | `GetTickTimer(spawnInterval)`(스테이지별) |
| 차선 구분선 애니 | 스크롤 오프셋 |
| 무적 깜빡 | 프레임 카운터(`invTimer`) |
| 스테이지/플레이타임 | `GetContentTime()` |

---

## 9. 렌더링 계획 (OnInGameRender)

1. **도로**: 좌/우 벽(`■` DARKGRAY), 차선 구분선(스크롤 점선), 도로면 비움.
2. **엔티티**: `entities`를 종류별 스프라이트·색으로. y 반올림해 셀 행으로.
3. **플레이어**: 차선 중앙에 십자 스프라이트. 무적 중이면 프레임 파리티로 깜빡(그리기 생략).
4. **HUD**(상단): SCORE, STAGE, HIGH / FUEL 바(10칸, 잔량별 색), SPEED. (하단) LIFE(`♥` 또는 숫자).
5. **오버레이**: `PAUSE`("-- PAUSE --"), `GAMEOVER`(결과: GAME OVER / SCORE / DISTANCE / HIGH SCORE / "PRESS SPACE TO RETRY").

> 성능: 매 프레임 전면 렌더로 시작(엔티티 수가 적어 부담 없음). 필요 시 변경 셀만 그리기/`WriteConsoleOutput` 최적화(폴리시).

---

## 10. 핵심 데이터 요약

```
playerLane / speed / fuel / life / invincible,invTimer : 플레이어 상태
entities[] (type, lane, y, w, h)                        : 적/트럭/아이템/장애물(값 타입 vector)
scrollAccum                                             : 스크롤 누적기
score / hiScore / distance / stage                      : 점수·거리·스테이지
gameState                                               : PLAYING/PAUSE/GAMEOVER
```

---

## 11. 구현 마일스톤

| # | 목표 | 산출물 |
|---|------|--------|
| M1 | **스캐폴딩** | RoadFighterContent 생성·등록(`_ECONTENT::ROADFIGHTER`), 도로/벽/차선 렌더, 플레이어 차선 이동 |
| M2 | **스크롤·적** | 속도(가속/감속), 스크롤, 일반 적 스폰·하강·제거 |
| M3 | **충돌·목숨** | AABB 충돌 → 목숨/무적/속도감소, 목숨 0 게임오버, 재시작 |
| M4 | **연료·점수** | 연료 감소·게임오버, 연료·보너스 아이템, 점수(주행/회피/아이템), HUD |
| M5 | **난이도·차량** | 스테이지(시간), 고속(위빙)·트럭, 장애물, 스폰 확률 |
| M6 | **연출·폴리시** | 타이틀(데모)·PAUSE·결과 화면, 무적 깜빡, 차선 구분선 애니, HIGH SCORE, 튜닝 |

**권장 순서**: M1~M3까지가 "플레이 가능한 최소 레이서"(달리고 피하고 죽는다). M4~M6이 완성도. 마일스톤마다 빌드(`MSBuild CSGP.sln /p:Configuration=Debug /p:Platform=x64`)로 검증.

---

## 12. 인코딩 · 주의사항 (CLAUDE.md / handover.md 연동)

- 신규 `.cpp`에 전각 글리프(`■ ◆ ★ ▲ ♥` 등)·한글 문자열을 넣으면 **반드시 CP949로 저장**. UTF-8 저장 시 콘솔 출력이 깨진다.
  - 사용 글리프는 모두 CP949(KS X 1001)에 존재: `■`(U+25A0), `◆`(U+25C6), `★`(U+2605), `▲`(U+25B2), `♥`(U+2665), `·`(U+00B7). 재인코딩 후 왕복 검증 권장(Dino 구현 방식 참고).
- 헤더는 ASCII(영문 주석) 권장. `framework.h`의 `_ECONTENT`에 `ROADFIGHTER`를 **끝에 추가**(정수 순서 유지).
- 좌표는 논리좌표로 계산하고 **출력 직전에만 `×2`**. `speed`/`fuel`/엔티티 `y`는 float로 관리, 렌더 시 반올림.
- 벽·차량 폭은 `LANE_COUNT`/`laneW`로부터 계산해 하드코딩을 피한다.

---

## 13. HUD 상세 (초안 §17)

| 위치 | 항목 | 표시 |
|------|------|------|
| 상단 y1 | SCORE | `SCORE 00012300` (8자리 zero-pad) |
| 상단 y1 | STAGE | `STAGE 2` |
| 상단 y1 | HIGH | `HIGH 00053400` |
| 상단 y2 | FUEL | `FUEL ███████░░░` (10칸, GREEN→YELLOW→RED) |
| 상단 y2 | SPEED | `SPEED 180km/h` |
| 하단 y23 | LIFE | `LIFE ♥ ♥ ♥` (또는 `LIFE 3`) |

---

## 14. 게임 오버 / 결과 (초안 §19)

```
        G A M E   O V E R

     SCORE     012350
     DISTANCE  4320m
     HIGH SCORE 53400

   PRESS SPACE TO RETRY
   PRESS ESC   TO TITLE
```
- SPACE → `StartGame()`로 재시작. ESC → 타이틀(초안의 "ESC EXIT"는 프레임워크 규칙상 타이틀→종료로 이어짐).

---

## 15. 향후 확장 (초안 §22, Out of Scope for MVP)

- 자유 횡이동(차선 스냅 대신 연속 이동)
- 야간 모드 / 비·눈길(마찰·시야) / 터널·다리
- 경찰차 추격, 오일 장애물(미끄러짐), 폭발 이펙트
- 최고기록 파일 저장/불러오기, 스테이지 선택, 무한 모드
- 콘솔 사운드(충돌/아이템/마일스톤 `Beep`)

---

*본 GDD는 CSGP 프레임워크(v0.2) 기준. 구현 착수 시 M1 스캐폴딩부터 진행 권장.*
