# BattleCityContent — Game Design Document (Console Battle City)

> CSGP(Console Game Pack) 프레임워크 기반 **콘솔 배틀 시티(BATTLE CITY)**. 탱크 액션/슈팅, Top View.
> `MazeContent`(맵 생성)·`GalagaContent`(오브젝트 풀링)와 동일 패턴(`IGameContent` 상속, `x*2` 좌표, `GetTickTimer`, 색 상수)을 따른다.
> 함께 볼 문서: [`CURRICULUM.md`](CURRICULUM.md), [`MazeContent_GDD.md`](MazeContent_GDD.md), [`GalagaContent_GDD.md`](GalagaContent_GDD.md), [`handover.md`](../handover.md), [`CLAUDE.md`](../CLAUDE.md).
> 이 문서는 **설계 명세**이며 아직 코드는 없다. 구현 순서는 §11 마일스톤 참고.

---

## 0. 이 게임의 설계 의도 (커리큘럼: **맵 생성 + 오브젝트 풀링 = 캡스톤**)

이 게임은 앞선 두 게임의 학습 축이 **한 게임에서 합쳐지는 종합 단계**다:

- [`MazeContent`](MazeContent_GDD.md)에서 배운 **맵 생성 알고리즘 + 연결성 검증(flood fill/BFS)** → 스테이지마다 전장(brick/steel/water/forest/ice/base)을 **절차적으로 생성**하고, 적 스폰이 기지까지 도달 가능한지 검증한다. (§6.2)
- [`GalagaContent`](GalagaContent_GDD.md)에서 배운 **오브젝트 풀(`Pool<T,N>`)** → 총알·적 탱크·폭발·아이템을 전부 풀로 관리한다. 실시간 탱크전은 총알·폭발이 훨씬 많아 풀링의 이점이 더 크다. (§6.1)

즉 배틀 시티는 **"절차적 맵 위에서, 풀링된 대량 오브젝트가 싸우는"** 두 기술의 결합 실습장이다. (보너스: 적 AI의 기지 추적에 Maze의 BFS 경로탐색을 재사용할 수 있다 — §6.4.)

> 스코프가 크다. 그래서 §11에서 Phase로 잘게 나눈다. Phase 1~2만으로도 "맵 위를 움직이며 벽을 부수고 적과 싸우는" 코어가 선다.

---

## 1. 개요

| 항목 | 내용 |
|------|------|
| 제목 | BATTLE CITY (CSGP 콘텐츠) |
| 장르 | 탱크 액션 / 슈팅 (실시간, Top View) |
| 플랫폼 | Windows 콘솔 (Win32 API) |
| 플레이 인원 | 1인 |
| 씬 클래스 | `BattleCityContent : public IGameContent` |
| 씬 enum | `_ECONTENT::BATTLECITY` (framework.h에 추가) |
| 목표 | 탱크를 조작해 적 탱크를 전멸시키고 **아군 기지(Eagle)를 방어**. 기지 파괴 또는 생명 0이면 게임 오버 |

핵심 재미: 파괴 가능한 지형을 활용한 공수 + 기지 방어의 긴장감. 엔지니어링 목표는 **절차적 맵 생성**과 **오브젝트 풀링**을 동시에 구현하는 것.

---

## 2. 핵심 게임 루프

```
입력 → 플레이어 이동/회전/발사 → 총알 이동 → 적 AI 이동/발사 → 적 스폰
   → 충돌 검사(총알-타일 / 총알-탱크 / 총알-총알 / 탱크-기지) → 파괴·점수 계산
   → 아이템 처리 → 화면 출력 → 다음 프레임
```

프레임워크 고정 루프(`OnUpdate → OnRender`, ~60fps) 위에서 **탱크/총알 이동은 각자의 틱 타이머**로, 발사 쿨다운·AI 방향전환·폭발 애니메이션도 `TIMER->GetTickTimer()`로 제어한다.

---

## 3. 화면 구성 & 좌표 매핑

논리 해상도 `GAME_SIZE 40×25`. 렌더 시 x는 항상 `×2`(전각 1칸=콘솔 2칸). `MainContent`가 최외곽 테두리를 그리므로 내부 x 1~38, y 1~23 사용. 전장은 **타일 격자**(타일 1개 = 글리프 1개)로 표현.

```
 논리좌표 기준 배치
 x:0        (전장 타일 1~24)                26        38
 ┌────────────────────────────────────────────────────┐ y=0 (MainContent 테두리)
 │ SCORE 000000  STAGE 1  LIFE ♥♥♥            ENEMY   │ y=1  상단 HUD
 │ ■■■■■■■■■■■■■■■■■■■■■■■■     ▼▼▼▼   │ y=2  ← 전장(steel 테두리) / 우측 남은적
 │ ■  □□    ≈≈≈      ♣♣      ■     ▼▼    │
 │ ■    ▼      □□□        ■■     ■             │
 │ ■  ♣♣           □□          ■     STAGE 1  │
 │ ■        □□     ■■■■        ■             │
 │ ■   ≈≈≈≈                    ■     HI 0000  │
 │ ■         ▲(player)   □□    ■             │
 │ ■             □□□□□        ■             │
 │ ■■■■■■■■■□★□■■■■■■■■■   │ y=21 ← 기지(★)는 벽돌 요새 안
 │                                                    │ y=22
 └────────────────────────────────────────────────────┘ y=24
```

### 배치 상수 (논리좌표)
| 상수 | 값 | 의미 |
|------|----|------|
| `FIELD_X0` / `FIELD_Y0` | 1 / 2 | 전장 타일 (0,0)의 논리 좌표 |
| `FIELD_W` / `FIELD_H` | 24 / 20 | 전장 타일 수 (외곽 1칸은 steel 테두리) |
| `HUD_X` | 52 (콘솔) | 우측 패널 (남은 적/스테이지/하이스코어) |

- 타일 `(tx, ty)` → 논리 `(FIELD_X0+tx, FIELD_Y0+ty)` → **콘솔 출력 `((FIELD_X0+tx)*2, FIELD_Y0+ty)`**.
- 탱크·총알은 float 좌표(부드러운 이동), 렌더 직전 반올림 → 타일 정렬.
- **전각/단각 혼용**: 전각(`▲ ▼ ● ■ □ ≈ ♣ ★ ♥`)은 2칸, 단각(`= *`)은 1칸(셀 왼쪽). 로직·충돌은 **타일 정수 좌표**로만. (Galaga GDD와 동일 규칙)

---

## 4. 조작 (InputManager)

| 키 | 동작 | 입력 방식 |
|----|------|-----------|
| `W / S / A / D` | 상/하/좌/우 이동 + 포신 방향 = 이동 방향 | `OnKeyStay` + 이동 틱 |
| `SPACE` | 발사 (Fire Delay 쿨다운, 동시 탄 수 = Power Level) | `OnKeyStay` + `GetTickTimer(fireDelay)` |
| `P` | 일시정지 토글 | `OnKeyDown` |
| `SPACE` / `Enter` | 스테이지 시작 / 게임오버 후 진행 | `OnKeyDown` |
| `ESC` | 인게임 → 타이틀, 타이틀 → 종료 | `OnKeyDown` — 기존 게임과 동일 규칙 |

> 탱크는 이동 방향으로 회전한다(포신=진행 방향). 벽에 막히면 제자리에서 방향만 바뀐다. 발사는 유지 중 쿨다운마다, **화면 위 동시 탄 수 상한**(기본 1, 스타 강화 시 2)을 넘지 않을 때만.

---

## 5. 타일 / 엔티티 글리프 정의

| 요소 | 글리프 | 색(framework.h) | 비고 |
|------|--------|------------------|------|
| 플레이어 탱크 | `▲` | `YELLOW`(14) | 방향에 따라 회전 표현(§9) |
| 적 탱크(기본) | `▼` | `GRAY`(7) | HP1 |
| 적 탱크(빠름) | `▼` | `SKYBLUE`(11) | HP1, 빠름 |
| 적 탱크(장갑) | `▼` | `GREEN`(10) | HP4 |
| 적 탱크(공격형) | `▼` | `RED`(12) | HP2, 사격↑ |
| 총알 | `●` | `WHITE`(15) | 방향 이동 |
| 벽(brick) | `□` | `DARKRED`(4)/`DARKYELLOW`(6) | 총알로 파괴 |
| 철벽(steel) | `■` | `GRAY`(7) | 강화탄만 파괴 |
| 물(water) | `≈` | `SKYBLUE`(11) | 탱크 불가, 총알 통과 |
| 숲(forest) | `♣` | `DARKGREEN`(2) | 통과 가능, 시야 가림 |
| 얼음(ice) | `=` | `WHITE`(15) | 미끄러짐(관성) |
| 기지(base) | `★` | `YELLOW`(14) | 피격 시 게임 오버 |
| 폭발(FX) | `*` → `+` → `X` | `YELLOW`→`RED` | 4~6프레임 |
| 생명 | `♥` | `RED`(12) | HUD |

> ⚠️ **글리프 인코딩 확인 필요**: 검증된 CP949 글리프는 `■ ● ◆ ★ ▲ ♥`. 그 외 `▼`(U+25BC)·`□`(U+25A1)·`≈`(U+2248)·`♣`(U+2663)는 KS X 1001에 있으나 **구현 전 왕복 검증 필수**. 실패 시 폴백 — 적 `▼`→`◆`, 벽 `□`→`▤`/`#`, 물 `≈`→`~`, 숲 `♣`→`♠`. 기지는 **검증된 `★`** 사용(스펙의 `☆`(U+2606) 미검증 회피). `= *`는 ASCII(단각). (§12)

---

## 6. 게임 시스템

### 6.1 ⭐ 오브젝트 풀 (기술 축 1)

Galaga와 동일하게 **재사용 가능한 제네릭 풀 `Pool<T, N>`**([GalagaContent_GDD.md](GalagaContent_GDD.md) §6.1) 위에서 총알·적·폭발·아이템을 돌린다. 실시간 탱크전은 총알/폭발이 훨씬 잦아 풀링 효과가 크다.

| 풀 | 상수 | 크기 | 스펙 대응 |
|----|------|------|-----------|
| 플레이어 총알 | `MAX_PBULLET` | 8 | BulletManager |
| 적 총알 | `MAX_EBULLET` | 24 | BulletManager |
| 적 탱크(동시) | `MAX_ENEMY` | 8 | EnemyManager |
| 폭발 FX | `MAX_FX` | 24 | (FX) |
| 아이템 | `MAX_ITEM` | 4 | ItemManager |

- **스폰**: `pool.Spawn()`으로 빈 슬롯 인덱스를 얻어 값 채움. **소멸**: `pool.Kill(i)`(플래그만). **순회**: `active`인 것만.
- 루프 중 `new/delete`·재할당 0회. 스테이지 전환/재시작은 각 풀 `Clear()`.
- 적 탱크는 **동시 최대 `MAX_ENEMY`(≤4~8)**, 스테이지당 총 20마리는 스폰 큐(`enemiesToSpawn`)로 관리 — 풀 크기와 총량을 분리.

### 6.2 ⭐ 맵 생성 알고리즘 (기술 축 2)

**스테이지마다 전장을 절차적으로 생성**한다. `_ETile tile[FIELD_H][FIELD_W]`에 다층 파이프라인으로 채운다:

1. **초기화**: 전부 `EMPTY`.
2. **철벽 테두리**: 외곽 1칸을 `STEEL`(파괴 불가 경계).
3. **기지 요새**: 하단 중앙에 `BASE(★)` 배치, 주변 3면을 `BRICK`으로 감싼다(이글의 벽돌 보호막).
4. **벽돌 군집(대칭 랜덤 배치)**: 무작위 사각/패턴 벽돌 덩어리를 **좌우 대칭**으로 배치(배틀 시티 특유의 대칭 맵). — *랜덤 배치 알고리즘*.
5. **철벽 블록**: 소수의 `STEEL` 덩어리(대칭).
6. **물/숲(셀룰러 오토마타)**: 무작위 노이즈를 뿌리고 **CA 스무딩**(이웃 다수결)으로 유기적 `WATER`/`FOREST` 영역 생성. — *셀룰러 오토마타*.
7. **얼음**: 소규모 `ICE` 패치 랜덤.
8. **스폰존 확보**: 상단 3개 스폰 지점, 플레이어 시작칸, 기지 정면을 `EMPTY`로 보장.
9. **연결성 검증(flood fill / BFS)**: 탱크 통행 가능 타일(EMPTY/FOREST/ICE) 기준으로 **각 적 스폰 → 기지/플레이어 도달 가능**한지 검사. 막혔으면 경로를 뚫거나 재생성. — **Maze의 flood-fill/BFS 재사용**.

> **스테이지 스케일**: 스테이지가 오를수록 벽돌↓·철벽↑·물/숲↑로 파라미터를 조절해 난이도를 준다(맵 생성 파라미터 = 난이도 다이얼). 원한다면 Maze처럼 "스테이지별 생성 스타일"을 enum으로 분기해도 된다.

### 6.3 플레이어 탱크 (스펙: Position/Direction/Life/Speed/Fire Delay/Power Level/Invincible Time)
- 이동: `WASD` 유지 시 이동 틱마다 1타일, 진행 방향으로 회전. 목적 타일이 통행 가능(EMPTY/FOREST/ICE)일 때만. `ICE` 위에서는 **관성**(다음 틱에 같은 방향 1칸 더 미끄러짐, 막히면 정지).
- 발사: `SPACE` + `GetTickTimer(fireDelay)`, 화면 위 내 총알 수 < (1 + powerLevel 보정)일 때 `pBullets.Spawn()`.
- **Power Level**: 스타 획득마다 ↑ — 동시 탄 수↑, 최대치에서 **강화탄**(철벽 파괴) 획득.
- 피격: `life--`, 무적(`invincible`, `invTimer`) + 폭발, `life<=0`이면 GAME OVER. 무적 중 깜빡·충돌 무시. 리스폰은 시작 지점.

### 6.4 적 탱크 & AI (Idle/Move/Attack/Dead)
- 종류: **기본**(HP1,보통·점수100) / **빠름**(HP1,빠름·200) / **공격형**(HP2,사격↑·300) / **장갑**(HP4,느림·400).
- **기본 AI**: 일정 시간마다 방향 랜덤 변경, 이동(벽이면 다른 방향), 주기적으로 진행/플레이어 방향 사격.
- **고급 AI(상위 스테이지)**: **기지 우선 접근** 또는 **플레이어 추적** — 목표 방향으로 편향 이동. *정밀하게 하려면 Maze의 BFS로 기지까지 다음 한 칸을 계산*(경로탐색 재사용).
- 일부 적은 **점멸(flashing)** — 파괴 시 아이템 드랍(§6.8).
- 파괴 연출: HP0 → 폭발 FX 스폰 후 `Kill`.

### 6.5 총알
- 플레이어/적 모두 `●`, 방향으로 **총알 틱(탱크보다 빠름)** 마다 이동.
- 명중 규칙:
  | 대상 | 결과 |
  |------|------|
  | `BRICK` | 벽돌 파괴(→EMPTY), 총알 소멸 |
  | `STEEL` | **강화탄이면** 파괴, 아니면 총알만 소멸 |
  | `WATER` | 통과(물 위로) |
  | `FOREST` | 통과 |
  | 탱크 | 피해(적↔내탄: 적 HP-1 / 내↔적탄: 생명-1) |
  | `BASE` | **게임 오버** |
  | 다른 총알 | 둘 다 소멸 |
  | 전장 밖 | 소멸 |

### 6.6 타일 상호작용 요약
| 타일 | 탱크 | 총알 | 특수 |
|------|------|------|------|
| BRICK `□` | 불가 | 파괴 | — |
| STEEL `■` | 불가 | 강화탄만 파괴 | 외곽=파괴불가 |
| WATER `≈` | 불가 | 통과 | — |
| FOREST `♣` | 가능 | 통과 | 위 오브젝트 **시야 가림**(숲을 나중에 덮어 그림) |
| ICE `=` | 가능 | 통과 | 관성(미끄러짐) |
| BASE `★` | 불가 | 파괴=패배 | 삽 아이템으로 일시 STEEL 보호 |

### 6.7 충돌 판정
- **총알 × 타일**: 총알의 다음 타일을 §6.5 규칙으로.
- **총알 × 탱크 / 총알 × 총알**: 활성 풀 순회 겹침. 내탄↔적탱크, 적탄↔플레이어, 내탄↔적탄(상쇄).
- **탱크 × 탱크**: 서로/기지 타일로는 진입 불가(막힘).
- **적탄/적탱크 × 기지**: 게임 오버.
- 판정은 타일 정수 좌표(AABB 1×1). 수가 커지면 타일 점유 맵으로 최적화(폴리시).

### 6.8 파워업 (점멸 적 파괴 시 드랍, `items` 풀)
| 아이템 | 효과 |
|--------|------|
| 별(star) | Power Level +1(동시탄↑ / 강화탄) |
| 헬멧(helmet) | 일정 시간 무적 |
| 시계(clock) | 모든 적 정지(`freezeTimer`) |
| 삽(shovel) | 기지 주변을 일시 STEEL로 보호(`shieldTimer`) |
| 탱크(tank) | Life +1 |
| 폭탄(bomb) | 활성 적 전멸 |
- 획득 시 점수 +500. 아이템 획득은 플레이어가 그 타일에 진입.

### 6.9 점수·생명·스테이지·적 스폰
- 점수: 기본 100 / 빠름 200 / 공격형 300 / 장갑 400 / 파워업 500 / 스테이지 클리어 +1000.
- 생명: 시작 3, 0이면 GAME OVER. HI 스코어 세션 유지(파일 저장은 §13/Phase4).
- 스테이지: 1=적20, 2=빠른탱크 등장, 3=장갑 등장, N=적 AI·속도·장애물 증가.
- 적 스폰: 상단 **3개 지점**, **동시 최대 4**(≤`MAX_ENEMY`), 스테이지당 총 20. `enemiesToSpawn`이 0이고 활성 적이 없으면 **STAGE CLEAR**.

### 6.10 폭발 애니메이션
- `fxs.Spawn(x,y)` → `frame`을 `GetTickTimer(0.06f)`마다 증가시켜 `*`→`+`→`X`로 바꾸고 4~6프레임 후 `Kill`. (선택) `Beep` 효과음.

---

## 7. 상태 머신

`IGameContent`의 페이즈(`TITLE/INGAME`)는 공용, 인게임 세부 상태는 `BattleCityContent` 내부 enum. 스펙의 7상태 매핑:

```
IGameContent::_EPhase
 ├─ TITLE   → OnTitleUpdate/Render  (BATTLE CITY 로고 + PRESS ENTER)
 └─ INGAME  → OnInGameUpdate/Render
       └─ BattleCityContent::_EGameState
            ├─ STAGESTART : "STAGE n" 짧은 인트로(맵 생성 완료 표시)
            ├─ PLAYING    : 정상 플레이
            ├─ PAUSE      : 일시정지(P)
            ├─ STAGECLEAR : "STAGE CLEAR +1000" → Enter로 다음 스테이지
            ├─ GAMEOVER   : "GAME OVER" (생명0 또는 기지파괴)
            └─ RESULT     : 스테이지 점수 요약 → 진행
```
- `ESC`(인게임) → `currentPhase = TITLE`. 타이틀 `ESC` → `main.cpp`가 종료.
- 스테이지/재시작은 풀 `Clear()` + `GenerateMap()` + 값 리셋(풀 재할당 없음).

---

## 8. 프레임워크 통합

### 8.1 추가/수정 파일
| 파일 | 작업 |
|------|------|
| `framework.h` | `enum class _ECONTENT`에 `BATTLECITY` 추가(끝에) |
| `Main/MainContent.cpp` | `SCENE->AddContent((int)_ECONTENT::BATTLECITY, new BattleCityContent());` 등록 |
| `Main/Content/BattleCityContent.h` | 신규 — `IGameContent` 상속 (ASCII 주석 권장) |
| `Main/Content/BattleCityContent.cpp` | 신규 — 전각 글리프 포함 시 **CP949 저장** |
| `CSGP.vcxproj` / `.filters` | 신규 두 파일 등록(UTF-8) |

### 8.2 스펙의 "개발 구조" → CSGP 매핑
게임은 **하나의 `IContent` 씬**. 공용 매니저는 프레임워크가 제공, 게임 고유 모듈은 씬 내부의 **타일 배열 + 풀 + 메서드**:

| 스펙 모듈 | CSGP 실현 |
|-----------|-----------|
| Input / Renderer | `INPUT` / `SCREEN` + `DrawXxx()` |
| **MapManager / TileManager** | **`_ETile tile[][]` + `GenerateMap()`(§6.2)** |
| Player | `px/pdir/life/...` + `UpdatePlayer()` |
| **EnemyManager / BulletManager** | **`Pool<Tank>` / `Pool<Bullet>` + `Update...()`** |
| CollisionManager | `HandleCollisions()` |
| StageManager | `stage/enemiesToSpawn` + `StartStage()` |
| ItemManager | `Pool<Item> items` + `UpdateItems()` |
| Score/UI Manager | `score/hiScore` + `DrawHUD()` |
| Save/Sound Manager | 파일 저장(§13) / `Beep()`(선택) |

### 8.3 클래스 설계 (헤더 스케치)
```cpp
class BattleCityContent : public IGameContent {
private:
    static const int FIELD_W = 24, FIELD_H = 20;
    static const int FIELD_X0 = 1, FIELD_Y0 = 2;

    static const int MAX_PBULLET = 8;
    static const int MAX_EBULLET = 24;
    static const int MAX_ENEMY   = 8;   // simultaneous
    static const int MAX_FX      = 24;
    static const int MAX_ITEM    = 4;

    enum class _EGameState { STAGESTART, PLAYING, PAUSE, STAGECLEAR, GAMEOVER, RESULT };
    enum class _ETile      { EMPTY, BRICK, STEEL, WATER, FOREST, ICE, BASE };
    enum class _EDir       { UP, RIGHT, DOWN, LEFT };
    enum class _EEnemyType { BASIC, FAST, ARMOR, ATTACK };
    enum class _EItemType  { STAR, HELMET, CLOCK, SHOVEL, TANK, BOMB };

    struct Bullet { float x, y; _EDir dir; bool power; bool fromPlayer; };
    struct Tank   { _EEnemyType type; float x, y; _EDir dir; int hp; float moveT, fireT; bool flashing; };
    struct Fx     { float x, y; int frame; };
    struct Item   { _EItemType type; int x, y; };

    _ETile tile[FIELD_H][FIELD_W];

    // --- object pools ---
    Pool<Bullet, MAX_PBULLET> pBullets;
    Pool<Bullet, MAX_EBULLET> eBullets;
    Pool<Tank,   MAX_ENEMY>   enemies;
    Pool<Fx,     MAX_FX>      fxs;
    Pool<Item,   MAX_ITEM>    items;

    // player
    float px, py; _EDir pdir;
    int   life, powerLevel; bool invincible; int invTimer;
    float pMoveT, pFireT;

    // base / progression
    int   baseX, baseY; bool baseAlive; int shieldTimer;
    int   score, hiScore, stage;
    int   enemiesToSpawn; float spawnT;
    int   freezeTimer;
    _EGameState gameState;

public:
    virtual void OnInit();  virtual void OnRelease();
    virtual void OnTitleUpdate();   virtual void OnTitleRender();
    virtual void OnInGameUpdate();  virtual void OnInGameRender();

private:
    void StartStage(int stage);

    // --- map generation (technique 2) ---
    void GenerateMap(int stage);
    void GenBorderAndBase();
    void GenBrickClusters();          // symmetric random placement
    void GenSteelBlocks();
    void GenWaterForestCA();          // cellular automata
    void GenIce();
    void ClearSpawnZones();
    bool VerifyConnectivity();        // flood fill: spawns -> base reachable

    bool Passable(int tx, int ty) const;
    void SpawnEnemy();
    void UpdatePlayer();  void UpdateBullets();  void UpdateEnemies();
    void UpdateItems();   void UpdateFx();
    void HandleCollisions();

    void DrawMap();   void DrawTanks();  void DrawBullets();
    void DrawItems(); void DrawFx();     void DrawForestOverlay();  void DrawHUD();
};
```
> `Pool<T,N>`는 공용 `Pool.h`(또는 헤더 상단)에 정의. **메모리 모델**: 타일 배열·모든 풀이 고정 크기 값 타입 — 생성자에서 잡히고 재사용, `OnRelease`는 사실상 비어 있다.

### 8.4 타이머 사용 요약
| 용도 | 방식 | 키(초, 예시) |
|------|------|--------|
| 플레이어 이동 | `GetTickTimer(playerMove)` | 0.10 |
| 총알 이동 | `GetTickTimer(bulletMove)` | 0.04 |
| 적 이동 | 종류별 `GetTickTimer(...)` | 0.10~0.20 |
| 발사 쿨다운 | `GetTickTimer(fireDelay)` | 0.30 |
| 적 스폰 | `GetTickTimer(spawn)` | 1.5 |
| 폭발 프레임 | `GetTickTimer(0.06f)` | 0.06 |
| 무적/정지/보호 타이머 | 프레임 카운트 | — |

---

## 9. 렌더링 계획 (OnInGameRender)

1. **맵 타일**: `tile[][]` (steel/brick/water/ice/base). 물/철벽/벽돌은 색으로 구분.
2. **아이템**: 활성 `items`.
3. **탱크**: 적 → 플레이어(`▲` 방향 회전). 무적 중 깜빡.
4. **총알**: 활성 `pBullets` + `eBullets`.
5. **폭발 FX**: 활성 `fxs`.
6. **숲 오버레이(`♣`)**: **탱크·총알 위에 다시 덮어** 그려 "시야 가림" 구현.
7. **HUD**: 상단 y=1 `SCORE/STAGE/LIFE ♥`, 우측 패널 `ENEMY 남은수(아이콘 ▼×n)`, `HI`.
8. **오버레이**: `STAGESTART`("STAGE n"), `PAUSE`, `STAGECLEAR`, `GAMEOVER`.

> 방향 회전 표현: MVP는 플레이어만 `▲▶▼◀`(검증 필요, 미검증이면 `▲`+방향 텍스트). 적은 `▼` 고정도 무방. 성능: 타일 24×20=480칸 + 오브젝트 — 행 문자열 묶어 그리기로 콘솔 호출 절감(Maze의 행-배치 렌더 참고).

---

## 10. 핵심 데이터 요약

```
tile[FIELD_H][FIELD_W] (_ETile)      : 전장 타일 (생성이 채우고 충돌/렌더가 읽음)
pBullets/eBullets (Pool<Bullet>)     : 총알 풀 (x,y,dir,power,fromPlayer)
enemies (Pool<Tank>)                 : 적 탱크 풀 (type,x,y,dir,hp,moveT,fireT,flashing)
items (Pool<Item>) / fxs (Pool<Fx>)  : 아이템 / 폭발 풀
px,py,pdir,life,powerLevel,invincible: 플레이어
baseX,baseY,baseAlive,shieldTimer    : 기지
stage,score,hiScore,enemiesToSpawn,freezeTimer : 진행
gameState                            : STAGESTART/PLAYING/PAUSE/STAGECLEAR/GAMEOVER/RESULT
```

---

## 11. 구현 마일스톤 (스펙 Phase 매핑)

| Phase | 목표 | 산출물 |
|:---:|------|--------|
| **1** | **맵 & 플레이어** | BattleCityContent 등록(`_ECONTENT::BATTLECITY`), **`GenerateMap`(테두리+기지+벽돌)**, 타일 렌더, 플레이어 이동/회전, 발사(`pBullets` **풀**), 총알-벽 충돌·벽돌 파괴 |
| **2** | **적 & 전투** | 적 스폰(3지점, `enemies` **풀**), 기본 AI 이동, 적 사격(`eBullets` 풀), 총알-탱크/총알-총알 충돌, 폭발(`fxs` 풀), 점수 |
| **3** | **기지 & 진행** | 기지 방어·패배 조건, 생명·무적, 스테이지 진행(20마리/스폰큐), STAGE CLEAR/GAME OVER, HUD |
| **4** | **완성** | **맵 생성 심화(물/숲 CA + 얼음 + 연결성 검증)**, 강화탄/파워업 6종(`items` 풀), 고급 AI(기지추적/BFS), 하이스코어 저장, **프리 리스트 O(1)** |

**권장 순서**: Phase 1~2가 "맵 위에서 벽 부수고 적과 싸우는" 코어(두 기술의 최소형: 벽돌맵 생성 + 총알/적 풀). Phase 4에서 맵 생성(CA/연결성)과 풀링(프리 리스트)을 완성한다. Phase마다 빌드(`MSBuild CSGP.sln /p:Configuration=Debug /p:Platform=x64`)로 검증.

### MVP 목표(스펙)
플레이어 이동·4방향 회전·발사·벽 파괴 · 적 AI 이동·적 제거 · 기지 방어 · 스테이지 진행 · 게임 오버 · 콘솔 실시간. — Phase 1~3으로 충족.

---

## 12. 인코딩 · 주의사항 (CLAUDE.md / handover.md 연동)

- 신규 `.cpp`에 전각 글리프·한글 문자열을 넣으면 **반드시 CP949로 저장**. 신규 소스는 **UTF-8 작성 → PowerShell로 CP949 재인코딩 + 글리프 왕복 검증** 권장(Dino/Maze/방식).
- **글리프 검증**: 검증된 세트 `■ ● ◆ ★ ▲ ♥`로 최대한 커버(steel `■`, 총알 `●`, 기지 `★`, 플레이어 `▲`, 생명 `♥`). 미검증 `▼ □ ≈ ♣`(및 방향 `▶◀`)는 **구현 시 왕복 검증** 후 사용, 실패 시 폴백(§5).
- **전각/단각 정렬**: 전각은 `logicalX*2`에 2칸, 단각(`= *`)은 1칸. 로직·충돌은 **타일 정수 좌표**로만.
- 헤더는 ASCII(영문 주석) 권장. `framework.h`의 `_ECONTENT`에 `BATTLECITY`를 **끝에 추가**(정수 순서 유지).
- **`Windows.h` 이름 충돌 주의**: `RED/GREEN/BLUE`는 색상수, `IN/OUT/NEAR/FAR/small` 등. enum/구조체는 `_E...`/`Tank/Bullet`처럼 안전하게. `_EDir::LEFT` 등 방향값이 VK와 헷갈리지 않게.
- **풀·타일은 고정 배열**. 스테이지 전환/재시작은 `Clear()` + `GenerateMap()`으로만, 재할당 금지.

---

## 13. 향후 확장 (Out of Scope for MVP)

- **2인 협동**(오리지널 특징) — 입력 분리 필요(프레임워크 확장).
- **맵 생성 심화**: 스테이지별 생성 스타일 enum(요새형/미로형/개활지형), 대칭 축 변경, 프리셋+절차 혼합.
- **적 AI 고도화**: BFS 기반 기지 최단 접근, 협공, 벽 우회 — Maze 경로탐색 재사용.
- **강화탄/관통탄·연사 업그레이드 트리**, 시간정지/폭탄 연출.
- **하이스코어·진행 저장**([CURRICULUM.md](CURRICULUM.md) §10 "저장/불러오기" — 스펙 SaveManager).
- **프리 리스트/세대 인덱스 풀**, 타일 점유 맵 기반 충돌 최적화.

---

*본 GDD는 CSGP 프레임워크(v0.2) 기준. 구현 착수 시 Phase 1(맵+플레이어)부터 진행 권장.*
