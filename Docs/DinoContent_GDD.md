# DinoContent — Game Design Document

> CSGP(Console Game Pack) 프레임워크 기반 **Chrome Dino(공룡 달리기)** 콘솔 이식.
> 원작: Google Chrome 오프라인 이스터에그(2014). 출처 정리 기반: 나무위키 「Chrome Dino」.
> `SnakeContent`/`TetrisContent`와 동일 패턴(`IGameContent` 상속, `x*2` 좌표, `GetTickTimer`, 색 상수)을 따른다.
> 함께 볼 문서: [`handover.md`](../handover.md), [`CLAUDE.md`](../CLAUDE.md), [`TetrisContent_GDD.md`](TetrisContent_GDD.md).
> 이 문서는 **설계 명세**이며 아직 코드는 없다. 구현 순서는 §11 마일스톤 참고.

---

## 1. 개요

| 항목 | 내용 |
|------|------|
| 제목 | DINO RUN (CSGP 콘텐츠) |
| 장르 | 러닝 액션 (엔들리스 러너) |
| 플랫폼 | Windows 콘솔 (Win32 API) |
| 플레이 인원 | 1인 |
| 씬 클래스 | `DinoContent : public IGameContent` |
| 씬 enum | `_ECONTENT::DINO` (framework.h에 추가) |
| 목표 | 좌→우로 스크롤되는 장애물(선인장/익룡)을 점프·숙이기로 피하며 최대한 오래 달려 고득점 |

핵심 재미: **한 버튼(점프)의 단순함 + 점점 빨라지는 속도**가 만드는 반사신경 긴장감. 원작의 미니멀한 흑백 픽셀 감성을 콘솔 블록 문자로 재현한다.

### 원작 메커니즘 → 콘솔 이식 취사선택
| 원작 요소 | 이식 | 비고 |
|-----------|------|------|
| 점프(스페이스/↑) | ✅ | 중력 기반 포물선 |
| 숙이기(↓) | ✅ | 중간 높이 익룡 회피 |
| 점프 중 ↓ = 빠른 착지(fast fall) | ✅ | 중력 배수 |
| 선인장(단일/묶음) | ✅ | 지면 장애물 |
| 익룡(여러 높이) | ✅ | 3높이 층(§5, §6.4) — 원작 각주[4] 반영 |
| 점수↑ → 속도↑ | ✅ | 거리 비례 점수 + 속도 가속 |
| 100점마다 소리+점수 깜빡 | ✅ | `Beep()`로 근사(선택), 점수 깜빡 |
| 700점마다 밤↔낮 전환(별·달) | ✅ | 색 팔레트 스왑 + 별/달 |
| 최고 점수(HI) | ✅ | 세션 내 유지(파일 저장은 향후확장) |
| 게임오버 후 재시작 | ✅ | 스페이스/엔터 |
| 일시정지(Alt) | ✅ | 콘솔에선 **P** 사용(Alt는 시스템 메뉴 유발) |
| 무기/올림픽/도넛/3D 등 변형 | ❌ | 이스터에그 파생, 범위 외 |
| 진동, 다크모드 반전 | ❌ | 콘솔 미지원 |

---

## 2. 핵심 게임 루프

```
달리기(자동 전진) → 오른쪽에서 장애물 스크롤 접근
   → 플레이어 점프/숙이기로 회피 → 통과 시 점수 누적 & 속도 가속
   → 장애물과 충돌하면 GAME OVER → 스페이스로 재시작
```

프레임워크 고정 루프(`OnUpdate → OnRender`, ~60fps) 위에서 **물리·스크롤은 프레임당 고정 스텝**으로 진행하고(속도 단위 = 칸/프레임), 애니메이션·마일스톤은 `TIMER->GetTickTimer()`로 제어한다.

---

## 3. 화면 레이아웃 & 좌표 매핑

논리 해상도 `GAME_SIZE 40×25`. 렌더 시 x는 항상 `×2`(전각 1칸=콘솔 2칸). `MainContent`가 최외곽 테두리를 그리므로 내부 x 1~38, y 1~23 사용.

```
 논리좌표 기준 배치 (y 아래로 증가)
 x:0                                              38
 ┌────────────────────────────────────────────────┐ y=0  (MainContent 테두리)
 │  HI 00512   00248                        ☾ *   │ y=1   점수/최고점수(우측), 밤엔 달·별
 │        ☁                    ☁                   │ y=3   구름(느린 스크롤)
 │                                                 │
 │                                                 │
 │     ▟▛                              ▲           │        점프 최고점 부근
 │     ██          (익룡 HIGH)  y≈13   ▲▲          │
 │     ▜▛ ← 공룡          (익룡 MID)  y≈15   ﹅     │
 │   (DINO_X=6)          (익룡 LOW)  y≈17  ♣  ♣♣   │        선인장(지면)
 │████████████████████████████████████████████████│ y=19  지면선
 │  ˙  ·   ˙    ·  ˙   ·     ˙   ·    ˙  ·   ˙     │ y=20  지면 질감(점선, 스크롤)
 └────────────────────────────────────────────────┘ y=24
```

### 배치 상수 (논리좌표)
| 상수 | 값 | 의미 |
|------|----|------|
| `GROUND_Y` | 19 | 지면선 행. 공룡·선인장이 서는 기준 |
| `DINO_X` | 6 | 공룡의 고정 x(항상 이 자리, 세계가 스크롤됨) |
| `DINO_FOOT` | 18 | 지면 위 공룡 발 기준 y (GROUND_Y-1) |
| 점프 최고점(발) | ≈ 10~11 | 물리 파라미터로 결정(§6.1) |

- 공룡은 x가 고정이고 **장애물·배경이 왼쪽으로 흘러** 달리는 느낌을 낸다.
- 셀 출력은 `OnDrawColor(logicalX * 2, y, ...)`. 텍스트(점수 등)는 콘솔 x를 직접 지정.

---

## 4. 조작 (InputManager)

| 키 | 동작 | 입력 방식 |
|----|------|-----------|
| `SPACE` 또는 `↑` | 점프 (지면에 있을 때만) | `OnKeyDown` |
| `↓` (유지) | 숙이기 / 점프 중이면 빠른 착지 | `OnKeyStay` |
| `P` | 일시정지 토글 | `OnKeyDown` |
| `SPACE` / `Enter` | 시작 / 게임오버 후 재시작 | `OnKeyDown` |
| `ESC` | 타이틀로 (인게임 → 타이틀) | `OnKeyDown` — TetrisContent와 동일 규칙 |

> 점프는 **엣지 트리거**(`OnKeyDown`)로 처리해 한 번 누름당 한 번만 점프. 숙이기는 **레벨**(`OnKeyStay`)로 누르는 동안 유지.
> ESC 뒤로가기 흐름(인게임→타이틀, 타이틀→종료)은 `main.cpp`가 `OnUpdate` 뒤에서 ESC를 검사하는 기존 구조를 그대로 활용한다([handover.md](../handover.md) §9 참고).

---

## 5. 엔티티 정의

원작은 단색 픽셀아트. 콘솔에선 전각 블록(`"■"`)으로 근사하고, 낮/밤 팔레트로 가독성을 준다(원작 느낌을 원하면 전부 `GRAY` 단색도 가능).

### 5.1 공룡(Dino)
- 상태: `RUN`(달리기), `JUMP`(점프/체공), `DUCK`(숙이기), `DEAD`.
- 크기(히트박스, 논리 셀):
  | 상태 | 폭 | 높이 | 발 기준 top y |
  |------|----|----|----|
  | 서있음(RUN/JUMP) | 3 | 3 | foot-2 |
  | 숙임(DUCK) | 4 | 2 | foot-1 |
- 달리기 애니: 다리 교차 2프레임(`GetTickTimer(0.1f)`로 토글). 점프 중엔 고정. `DEAD`는 눈 커진 프레임(원작 오마주).
- 색: 낮 `GRAY`(7)/`WHITE`(15), 밤 `DARKGRAY`(8).

### 5.2 장애물(Obstacle)
| 타입 | 배치 | 폭×높이 | 회피법 |
|------|------|---------|--------|
| `CACTUS_SMALL` | 지면 | 1×2 | 점프 |
| `CACTUS_LARGE` | 지면 | 2×3 (묶음 1~3개) | 점프 |
| `PTERO_LOW` | 공중 y≈17 | 3×2 | **점프 필수** |
| `PTERO_MID` | 공중 y≈15 | 3×2 | 숙이기(또는 점프) |
| `PTERO_HIGH` | 공중 y≈13 | 3×2 | 서있어도 안전(무동작 통과) |
- 익룡은 날개 2프레임 애니. 색: 낮 `GRAY`, 밤 `DARKGRAY`.
- 선인장 색: 원작 충실은 `GRAY`, 가독성 우선이면 `GREEN`(10) 선택.

### 5.3 배경(장식, 게임플레이 무관)
- `Cloud`: 상단(y 2~5)에서 **공룡 속도보다 느리게** 스크롤(시차). 색 `DARKGRAY`.
- `Star`/`Moon`: 밤에만. 별은 `WHITE` 점, 달은 `YELLOW`/`WHITE`. 원작처럼 밤마다 달 모양(위상)을 바꿔도 좋음(향후).
- 지면 질감: y=20 라인에 점(`·`)을 스크롤시켜 이동감 강조.

---

## 6. 게임 시스템

### 6.1 공룡 물리 (프레임 기반)
`dinoY`(발의 y, float), `dinoVelY`(float)로 관리. **매 프레임(OnUpdate) 고정 스텝** 적용:
```
dinoVelY += G          // 중력
dinoY    += dinoVelY
if (dinoY >= DINO_FOOT) { dinoY = DINO_FOOT; dinoVelY = 0; onGround = true; }  // 착지
```
- **점프**: 지면(`onGround`)에서 SPACE/↑ → `dinoVelY = JUMP_V; onGround = false;`
- **빠른 착지(fast fall)**: 체공 중 ↓ 유지 → 이 프레임 중력을 `G * FASTFALL_MULT`로.
- **숙이기**: 지면에서 ↓ 유지 → 상태 `DUCK`(히트박스 낮고 넓게). 공중에선 fast fall로 해석.

**초기 제안 수치** (칸/프레임 단위, 플레이하며 튜닝):
| 파라미터 | 값 | 근거 |
|----------|----|------|
| 중력 `G` | 0.06 | — |
| 점프 초속 `JUMP_V` | -0.95 | 최고 상승 ≈ V²/2G ≈ 7.5칸, 발 최고 y≈10.5 |
| 체공 시간 | ≈ 0.53s (≈32프레임) | 2·|V|/G |
| fast fall 배수 | 3.0 | 원작의 빠른 착지 |

### 6.2 지면 & 월드 스크롤
- `gameSpeed`(칸/프레임)만큼 매 프레임 모든 장애물·배경 x를 감소.
- 화면 왼쪽(`x < 0`)을 벗어난 장애물은 제거.

### 6.3 난이도(속도) 증가
```
gameSpeed = min(SPEED_MAX, SPEED_0 + score * SPEED_ACCEL)
```
| 파라미터 | 값 |
|----------|----|
| `SPEED_0` (시작) | 0.45 |
| `SPEED_MAX` | 1.2 |
| `SPEED_ACCEL` | 0.0004 (점수당) |
- 속도가 오르면 같은 시간에 더 많이 전진 → 점수도 빨라짐(원작 서술과 일치).

### 6.4 장애물 스폰
- **거리 기반**: `spawnGap`(다음 스폰까지 남은 거리)을 매 프레임 `gameSpeed`만큼 차감, 0 이하가 되면 오른쪽 끝(x≈38)에 장애물 생성 후 `spawnGap`을 새로 뽑음.
- **최소 간격 보장**: `gapMin`은 "점프 체공으로 넘을 수 있는 거리"보다 커야 함(속도 비례). `spawnGap = gapMin + rand()%gapRand`.
- **타입 확률(점수 게이팅)** — 원작: 처음엔 선인장만, 200~500점 이후 익룡:
  - `score < 200`: 선인장만
  - `score >= 200`: 선인장 70% / 익룡 30% (익룡은 LOW/MID/HIGH 중 랜덤)
- 익룡 HIGH는 "무동작 통과" 가능하게 배치해 원작 각주[4]("점프하지 않고 지나가는 경우")를 재현.

### 6.5 충돌 (AABB)
- 공룡 히트박스(상태별 §5.1) vs 각 장애물 히트박스의 축정렬 사각형 겹침 검사.
- UX를 위해 히트박스를 실제 그림보다 1칸 정도 **관대하게(작게)** 잡는다.
- 충돌 시 `dinoState = DEAD`, `gameState = GAMEOVER`, HI 스코어 갱신.

### 6.6 점수 · 마일스톤 · 최고 점수
- **점수**: 거리 비례 누적. `scoreF += gameSpeed * SCORE_K; score = (int)scoreF;` (`SCORE_K ≈ 2`).
- **100점 마일스톤**: `score/100`이 증가하는 순간 → 점수 텍스트 잠깐 깜빡(0.1s 토글 몇 회) + `Beep(1000, 60)`(선택; 콘솔 사운드, 짧게).
- **최고 점수(HI)**: 세션 동안 멤버로 유지, 우측 상단 `HI 00512` 형식. 파일 저장은 향후확장.

### 6.7 밤 / 낮 전환
- `nightPhase = (score / 700) % 2` → 0=낮, 1=밤. 700 배수마다 토글(원작).
- 전환 시 팔레트 스왑: 낮(밝은 전경) ↔ 밤(어두운 전경 + 별/달). 콘솔 기본 배경이 검정이라 완전 반전 대신 **전경 색 팔레트**만 바꾼다.
- 밤에는 상단에 별 몇 개(`WHITE`)와 달(`YELLOW`) 표시.

### 6.8 애니메이션 타이머
| 대상 | 방식 |
|------|------|
| 공룡 달리기 다리 | `GetTickTimer(0.1f)` 토글(2프레임) |
| 익룡 날개 | `GetTickTimer(0.15f)` 토글(2프레임) |
| 점수 깜빡(마일스톤) | `GetTickTimer(0.1f)` 몇 회 |

---

## 7. 상태 머신

`IGameContent`의 페이즈(`TITLE/INGAME`)는 공용이라 수정하지 않고, 인게임 세부 상태는 `DinoContent` 내부 enum으로 관리한다(TetrisContent와 동일 전략).

```
IGameContent::_EPhase
 ├─ TITLE   → OnTitleUpdate/Render  (DINO 로고 + PRESS SPACE, 달리는 공룡 데모)
 └─ INGAME  → OnInGameUpdate/Render
       └─ DinoContent::_EGameState
            ├─ READY     : "SPACE로 시작" 대기(첫 입력 시 PLAYING)
            ├─ PLAYING   : 정상 플레이
            ├─ PAUSE     : 일시정지 오버레이(P 토글)
            └─ GAMEOVER  : "GAME OVER" + 재시작 아이콘, SPACE/Enter로 재시작
```
- `ESC`(인게임) → `currentPhase = TITLE`. 타이틀에서 `ESC` → `main.cpp`가 종료(기존 규칙).
- 재시작(GAMEOVER→PLAYING)은 `StartGame()`으로 월드 리셋.

---

## 8. 프레임워크 통합

### 8.1 추가/수정 파일
| 파일 | 작업 |
|------|------|
| `framework.h` | `enum class _ECONTENT`에 `DINO` 추가(끝에) |
| `Main/MainContent.cpp` | `SCENE->AddContent((int)_ECONTENT::DINO, new DinoContent());` 등록 |
| `Main/Content/DinoContent.h` | 신규 — `IGameContent` 상속 (ASCII 주석 권장) |
| `Main/Content/DinoContent.cpp` | 신규 — 전각/한글 포함 시 **CP949 저장** |
| `CSGP.vcxproj` / `.filters` | 신규 두 파일 등록(UTF-8, Edit 가능) |

### 8.2 클래스 설계 (헤더 스케치)
```cpp
class DinoContent : public IGameContent {
private:
    static const int GROUND_Y  = 19;
    static const int DINO_X    = 6;
    static const int DINO_FOOT = 18;

    enum class _EGameState { READY, PLAYING, PAUSE, GAMEOVER };
    enum class _EDinoState { RUN, JUMP, DUCK, DEAD };
    enum class _EObstacle  { CACTUS_SMALL, CACTUS_LARGE, PTERO_LOW, PTERO_MID, PTERO_HIGH };

    struct Obstacle { int type; float x; int w, h; int topY; };
    struct Cloud    { float x; int y; };

    // 공룡
    float dinoY;        // 발 y (float)
    float dinoVelY;
    _EDinoState dinoState;
    bool  onGround;

    std::vector<Obstacle> obstacles;
    std::vector<Cloud>    clouds;

    float gameSpeed;
    float scoreF;
    int   score, hiScore;
    int   nightPhase;
    float spawnGap;
    int   animFrame;

    _EGameState gameState;

public:
    virtual void OnInit();
    virtual void OnRelease();
    virtual void OnTitleUpdate();   virtual void OnTitleRender();
    virtual void OnInGameUpdate();  virtual void OnInGameRender();

private:
    void StartGame();
    void UpdateDino();              // 물리(점프/중력/숙이기/fast fall)
    void UpdateWorld();             // 스크롤·스폰·제거·속도
    void SpawnObstacle();
    bool CheckCollision();          // AABB
    void UpdateScore();             // 점수·마일스톤·밤낮

    void DrawDino();
    void DrawObstacle(const Obstacle& o);
    void DrawGround();
    void DrawBackground();          // 구름/별/달
    void DrawSprite(int cx, int cy, const char* rows[], int h, int color); // 블록 스프라이트 헬퍼
};
```
> **메모리 모델**: 장애물/구름을 값 타입 `std::vector`로 관리(raw 포인터·수동 delete 회피). `OnRelease`는 벡터 clear 정도로 최소화.

### 8.3 타이머 사용 요약
| 용도 | 방식 |
|------|------|
| 물리·스크롤 | 매 프레임 고정 스텝(칸/프레임) |
| 공룡 다리 애니 | `GetTickTimer(0.1f)` |
| 익룡 날개 애니 | `GetTickTimer(0.15f)` |
| 점수 깜빡 | `GetTickTimer(0.1f)` |
| 플레이 타임 표시(선택) | `GetContentTime()` |

---

## 9. 렌더링 계획 (OnInGameRender)

1. **배경**: (밤이면) 별·달 → 구름. 색 팔레트는 `nightPhase`로 결정.
2. **지면**: `GROUND_Y` 라인 + y=20 점선 질감(스크롤 오프셋 반영).
3. **장애물**: 각 `Obstacle`을 블록 스프라이트로. 익룡은 `animFrame`으로 날개 토글.
4. **공룡**: 상태별 스프라이트(`RUN` 다리 애니 / `JUMP` 고정 / `DUCK` 납작 / `DEAD` 눈 큼). `dinoY` 반올림해 셀 y로.
5. **UI**: 우측 상단 `HI xxxxx`(있으면) + 현재 점수(5자리 zero-pad, `std::to_string`). 마일스톤 깜빡 시 토글.
6. **오버레이**: `READY`("SPACE to start"), `PAUSE`("-- PAUSE --"), `GAMEOVER`("GAME OVER" + "PRESS SPACE").

> 성능: 매 프레임 전면 렌더로 시작(장애물 수가 적어 부담 없음). 필요 시 변경 셀만 그리기/`WriteConsoleOutput`로 최적화(폴리시).

---

## 10. 핵심 데이터 요약

```
dinoY / dinoVelY / dinoState / onGround   : 공룡 물리·상태
obstacles[] (type,x,w,h,topY)             : 활성 장애물(값 타입 vector)
clouds[]                                  : 배경 구름
gameSpeed                                 : 현재 전진 속도(칸/프레임)
scoreF / score / hiScore                  : 점수(실수 누적 → 정수), 최고 점수
nightPhase                                : 0 낮 / 1 밤
spawnGap                                  : 다음 스폰까지 남은 거리
animFrame                                 : 애니메이션 토글
gameState                                 : READY/PLAYING/PAUSE/GAMEOVER
```

---

## 11. 구현 마일스톤

| # | 목표 | 산출물 |
|---|------|--------|
| M1 | **스캐폴딩** | DinoContent 생성·등록(`_ECONTENT::DINO`), 지면 + 서있는 공룡 렌더, 지면 질감 스크롤 |
| M2 | **물리** | 점프(중력/착지), 숙이기, fast fall, 달리기 다리 애니 |
| M3 | **장애물** | 선인장 스폰/스크롤/제거, AABB 충돌 → GAMEOVER, 재시작 |
| M4 | **진행** | 점수·속도 가속, 익룡(3높이) 점수 게이팅, 익룡 날개 애니 |
| M5 | **연출** | 밤/낮(700점) 팔레트·별·달, 100점 마일스톤(깜빡+Beep), HI 스코어, 구름 시차 |
| M6 | **상태·폴리시** | 타이틀(달리는 데모)·READY·PAUSE·GAMEOVER, ESC 타이틀, 수치 튜닝 |

**권장 순서**: M1~M3까지가 "플레이 가능한 최소 러너"(달리고 점프해 선인장 피하고 죽음). M4~M6이 완성도. 마일스톤마다 빌드(`MSBuild CSGP.sln /p:Configuration=Debug /p:Platform=x64`)로 검증.

---

## 12. 인코딩 · 주의사항 (CLAUDE.md / handover.md 연동)

- 신규 `.cpp`에 전각 글리프(`"■"` 등)·한글 문자열을 넣으면 **반드시 CP949로 저장**(기존 소스 규칙). UTF-8 저장 시 콘솔 출력이 깨진다.
- 헤더는 ASCII(영문 주석) 권장. `framework.h`의 `_ECONTENT`에 `DINO`를 **끝에 추가**(정수 순서 유지).
- 좌표는 논리좌표로 계산하고 **출력 직전에만 `×2`**. `dinoY`는 float로 물리, 렌더 시 반올림.
- `Beep()`는 블로킹 호출이라 아주 짧게(≤60ms)만, 마일스톤에서만 사용. 프레임 끊김이 거슬리면 생략.

---

## 13. 향후 확장 (Out of Scope for MVP)

- 최고 점수 파일 저장/불러오기(세션 간 유지)
- 달 위상 변화, 낮/밤 페이드 전환
- 위아래로 움직이는 익룡, 선인장+익룡 복합 패턴
- 도쿄 올림픽/무기/도넛 등 원작 이스터에그 변형 테마
- 콘솔 사운드 강화(마일스톤/점프/사망 효과음)

---

*본 GDD는 CSGP 프레임워크(v0.2) 기준. 구현 착수 시 M1 스캐폴딩부터 진행 권장.*
