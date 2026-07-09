# GalagaContent — Game Design Document (Console Galaga)

> CSGP(Console Game Pack) 프레임워크 기반 **콘솔 갤러그(GALAGA)**. 슈팅 아케이드.
> `SnakeContent`/`TetrisContent`/`DinoContent`/`MazeContent`와 동일 패턴(`IGameContent` 상속, `x*2` 좌표, `GetTickTimer`, 색 상수)을 따른다.
> 함께 볼 문서: [`CURRICULUM.md`](CURRICULUM.md), [`handover.md`](../handover.md), [`CLAUDE.md`](../CLAUDE.md), [`MazeContent_GDD.md`](MazeContent_GDD.md).
> 이 문서는 **설계 명세**이며 아직 코드는 없다. 구현 순서는 §11 마일스톤 참고.

---

## 0. 이 게임의 설계 의도 (커리큘럼: **Object Pool 학습**)

이 게임의 배움 주제는 딱 하나다 — **오브젝트 풀(Object Pool)**.

[`CURRICULUM.md`](CURRICULUM.md)의 게임들은 오브젝트를 순진하게 관리했다: Snake는 마디마다 `new/delete`, Dino·Road Fighter는 **매 프레임 벡터를 통째로 재생성**(`keptO.push_back`). 오브젝트가 적을 땐 괜찮지만, 갤러그처럼 **총알·폭발이 초당 수십 개 생겼다 사라지는** 게임에서는 매 프레임 힙 할당/해제가 병목이 된다.

**해법 = 오브젝트 풀:**
> 고정 크기 배열(풀)을 미리 잡아 두고 `active` 플래그로 켜고 끄며 슬롯을 **재사용**한다. 게임 루프 안에서 `new`/`delete`/재할당이 **0회**.

이 GDD는 풀을 **재사용 가능한 제네릭 템플릿 `Pool<T, N>`** 으로 만들어 개념을 눈에 보이게 한다(§6.1). 총알·적·폭발이 모두 이 풀 위에서 돈다. 즉 갤러그는 "Object Pool을 손으로 만들어 보고, 여러 곳에 붙여 보는" 실습장이다.

---

## 1. 개요

| 항목 | 내용 |
|------|------|
| 제목 | GALAGA (CSGP 콘텐츠) |
| 장르 | 슈팅 아케이드 (실시간) |
| 플랫폼 | Windows 콘솔 (Win32 API) |
| 플레이 인원 | 1인 |
| 씬 클래스 | `GalagaContent : public IGameContent` |
| 씬 enum | `_ECONTENT::GALAGA` (framework.h에 추가) |
| 목표 | 우주선을 조작해 현재 스테이지의 적을 **모두 제거** → 다음 스테이지. 피격/충돌로 생명 감소, 생명 0이면 게임 오버 |

핵심 재미: 편대를 이룬 적을 정리하는 아케이드 손맛 + 스테이지마다 늘어나는 적·속도·탄. 엔지니어링 목표는 **모든 단명 오브젝트를 풀로 관리**하는 것.

---

## 2. 핵심 게임 루프

```
입력 → 플레이어 이동 → 총알 이동 → 적 편대 이동 → 적 공격
   → 충돌 검사(내탄-적 / 적탄-나 / 적-나) → 점수 계산 → 화면 출력 → 다음 프레임
```

프레임워크 고정 루프(`OnUpdate → OnRender`, ~60fps) 위에서 **이동/탄은 프레임당 고정 스텝**(속도=칸/프레임), 편대 이동·사격 쿨다운·폭발 애니메이션은 `TIMER->GetTickTimer()`로 제어한다.

---

## 3. 화면 구성 & 좌표 매핑

논리 해상도 `GAME_SIZE 40×25`. 렌더 시 x는 항상 `×2`(전각 1칸=콘솔 2칸). `MainContent`가 최외곽 테두리를 그리므로 내부 x 1~38, y 1~23 사용.

```
 논리좌표 기준 배치 (y 아래로 증가)
 x:0                                              38
 ┌────────────────────────────────────────────────┐ y=0  (MainContent 테두리)
 │ SCORE 000000   HI 001500   STAGE 1   LIFE ♥♥♥ │ y=1   상단 HUD
 │   ▼    ▼    ▼    ▼    ▼    ▼                    │ y=3   ← 적 편대(formation)
 │      ▼    ▼    ▼    ▼    ▼                       │ y=5
 │   ▼    ▼    ▼    ▼    ▼    ▼                    │ y=7
 │            !         !                           │       적 탄(!) 아래로
 │                 |                               │       내 탄(|) 위로
 │              |                                  │
 │                                                 │
 │                   ▲   ← 플레이어(y=21, x만 이동) │ y=21
 │══════════════════════════════════════════════ │ y=22  구분선
 │  A ◀ 이동 ▶ D    SPACE 발사    P 정지  ESC 뒤로 │ y=23  조작 안내
 └────────────────────────────────────────────────┘ y=24
```

### 배치 상수 (논리좌표)
| 상수 | 값 | 의미 |
|------|----|------|
| `HUD_Y` | 1 | 상단 정보줄 |
| `FORM_TOP` | 3 | 편대 최상단 행 |
| `PLAY_LEFT` / `PLAY_RIGHT` | 1 / 37 | 좌우 이동 한계 |
| `PLAYER_Y` | 21 | 플레이어 고정 y (x만 이동) |
| `DIVIDER_Y` | 22 | 구분선 |
| `CONTROL_Y` | 23 | 조작 안내 |

### 전각 / 단각 혼용 규칙 (중요)
- **전각 글리프**(`▲ ▼ ◆ ■ ♥`)는 콘솔 2칸을 차지 → `OnDrawColor(logicalX * 2, y, ...)`.
- **단각 글리프**(`| ! * × +`)는 1칸 → 같은 `logicalX * 2` 위치에 그리되 셀의 **왼쪽 칸**에 표시된다. 게임 로직·충돌은 **논리 정수 좌표**로만 계산하고, 표현 차이는 렌더에서만 흡수한다.
- 총알은 x 고정·y만 이동(위/아래 직선). y는 float로 이동, 렌더 시 반올림.

---

## 4. 조작 (InputManager)

| 키 | 동작 | 입력 방식 |
|----|------|-----------|
| `A` | 좌측 이동 | `OnKeyStay` |
| `D` | 우측 이동 | `OnKeyStay` |
| `SPACE` | 발사 (Fire Delay 쿨다운) | `OnKeyStay` + `GetTickTimer(fireDelay)` |
| `P` | 일시정지 토글 | `OnKeyDown` |
| `SPACE` / `Enter` | 게임오버 후 재시작 / 스테이지 클리어 후 진행 | `OnKeyDown` |
| `ESC` | 인게임 → 타이틀, 타이틀 → 종료 | `OnKeyDown` — 기존 게임과 동일 규칙 |

> 이동은 **레벨 입력**(`OnKeyStay`), 발사는 유지 중 `GetTickTimer(fireDelay)`가 true일 때만 → 연사 속도 제한.
> 스펙의 "ESC 종료"는 프레임워크 관례(인게임→타이틀, 타이틀→종료)로 실현한다. `main.cpp`가 `OnUpdate` 뒤에서 ESC를 검사([handover.md](../handover.md) §8). (원한다면 `←/→`도 이동 보조키로 추가 가능.)

---

## 5. 엔티티/글리프 정의

| 요소 | 글리프 | 색(framework.h) | 비고 |
|------|--------|------------------|------|
| 플레이어 | `▲` | `SKYBLUE`(11) | 하단 고정 y, x만 이동, 히트박스 1×1 |
| 기본 적 | `▼` | `RED`(12) | HP1 |
| 빠른 적 | `▼` | `GREEN`(10) | HP1, 빠른 이동/공격 |
| 강한 적 | `▼` | `PURPLE`(13) | HP2~3, 탄막 |
| 플레이어 총알 | `\|` | `YELLOW`(14) | 위로(vy<0) |
| 적 총알 | `!` | `RED`(12) | 아래로(vy>0) |
| 폭발(FX) | `*` → `×` → `+` | `YELLOW`→`DARKYELLOW` | 4~6프레임 후 소멸 |
| 생명 표시 | `♥` | `RED`(12) | HUD, 남은 생명 수만큼 |

> 적 종류는 **색**으로 구분(글리프는 모두 `▼`). 원하면 강한 적만 `■`(전각, 검증됨) 등으로 대체 가능.
> ⚠️ **글리프 인코딩 확인 필요**: 검증된 CP949 글리프는 `■ ● ◆ ★ ▲ ♥`이다. `▼`(U+25BC)·`×`(U+00D7)는 CP949(KS X 1001)에 있으나 **구현 전 왕복 검증 필수**. 실패 시 폴백 — 적 `▼`→`◆`, 폭발 `×`→`x`. `| ! * +`는 ASCII라 안전(단각). (§12)

---

## 6. 게임 시스템

### 6.1 ⭐ 오브젝트 풀 (이 게임의 핵심 학습)

**재사용 가능한 제네릭 풀 템플릿을 하나 만들고, 총알/적/폭발이 모두 그 위에서 돈다.**

```cpp
// 학습용 제네릭 오브젝트 풀. 고정 크기 N, active 플래그로 재사용.
template <typename T, int N>
struct Pool
{
    T    items[N];
    bool active[N];

    Pool()        { for (int i = 0; i < N; i++) active[i] = false; }
    void Clear()  { for (int i = 0; i < N; i++) active[i] = false; }

    int Spawn()   // 빈 슬롯을 찾아 켠다. 없으면 -1.
    {
        for (int i = 0; i < N; i++)
            if (!active[i]) { active[i] = true; return i; }
        return -1;
    }
    void Kill(int i) { active[i] = false; }   // 해제 아님, 표시만
};
```

사용 패턴 — **스폰/소멸/순회 모두 할당 없음**:
```cpp
Pool<Bullet, MAX_PBULLET> pBullets;

// 스폰: 슬롯을 빌려 채운다
int i = pBullets.Spawn();
if (i >= 0) pBullets.items[i] = { px, PLAYER_Y - 1, 0, -BULLET_SPEED };

// 순회: active인 것만 갱신 (화면 밖이면 Kill)
for (int k = 0; k < MAX_PBULLET; k++)
{
    if (!pBullets.active[k]) continue;
    pBullets.items[k].y += pBullets.items[k].vy;
    if (pBullets.items[k].y < FORM_TOP - 1) pBullets.Kill(k);
}
```

**풀 목록 & 용량(초기 제안)** — 스펙의 `BulletManager`/`EnemyManager`가 곧 이 풀들이다:
| 풀 | 상수 | 크기 | 스펙 대응 |
|----|------|------|-----------|
| 플레이어 총알 | `MAX_PBULLET` | 32 | BulletManager |
| 적 총알 | `MAX_EBULLET` | 64 | BulletManager |
| 적 | `MAX_ENEMY` | 40 | EnemyManager |
| 폭발 FX | `MAX_FX` | 32 | (Renderer/FX) |

- **게임 루프 안에서 `new/delete`·`push_back` 재할당 0회** → 할당 비용·단편화·복사 제거, 캐시 친화적, 최악 비용이 상한으로 예측 가능.
- **재시작/스테이지 전환**: 각 풀 `Clear()`(전부 `active=false`)만. 풀은 재할당하지 않는다 — 이 게임의 취지.

> **대조 학습**: Dino/Road Fighter의 "매 프레임 벡터 재생성"과 나란히 두고, 적/총알 수를 늘려가며 체감 차이를 관찰.
> **폴리시(심화 학습) — 프리 리스트**: `Spawn()`의 선형 탐색(O(N))을 비활성 인덱스 스택으로 바꾸면 O(1). MVP는 선형 탐색으로 시작 → **"측정 후 최적화"** 를 직접 경험.

### 6.2 플레이어 (스펙: Position X / Life / Score / Fire Delay / Invincible Time)
- 이동: `A/D` 유지 시 x를 `speed`만큼, `PLAY_LEFT~PLAY_RIGHT`로 클램프. y는 `PLAYER_Y` 고정.
- 발사: `SPACE` 유지 + `GetTickTimer(fireDelay)`가 true일 때 `pBullets.Spawn()`으로 위로 1발.
- 피격: `life--`, 무적(`invincible`, `invTimer` 프레임) 부여, 폭발 FX 스폰, `life<=0`이면 GAME OVER. 무적 중엔 깜빡이며 충돌 무시.

### 6.3 적 & 편대 (Idle / Move / Attack / Dead)
- 적은 **편대 슬롯** `(col, row)`을 갖고 풀에 담긴다. 실제 위치는 편대 상태에서 파생 → **"편대 유지"**:
  ```
  enemyX = formX + col * SPACING_X
  enemyY = FORM_TOP + formY + row * SPACING_Y
  ```
- **편대 이동(Move)**: `GetTickTimer(formTick)`마다 `formX += dir`. 편대의 좌/우 끝이 벽에 닿으면 `dir` 반전 + `formY += 1`(한 칸 하강). (스페이스 인베이더식 행진 = "좌우 이동 + 아래 이동".)
- **공격(Attack)**: `GetTickTimer(attackTick)`마다 살아있는 적 중 랜덤 하나가 자기 위치에서 `eBullets.Spawn()`으로 아래로 발사. 강한 적은 3방향 확산(탄막).
- **상태**: Idle(스폰 직후) → Move(행진) → Attack(사격 순간) → Dead(HP0 → FX 스폰 후 `Kill`).

> **갤러그 맛 폴리시(§13)**: 일부 적이 편대를 이탈해 **다이브 공격**(자기 x/y 궤적으로 하강 후 복귀). 이때는 슬롯 대신 절대 좌표로 전환. MVP는 편대 고정.

### 6.4 적 종류
| 종류 | 색 | HP | 행동 | 점수 |
|------|----|----|------|------|
| 기본 적 | RED | 1 | 좌우 이동 + 가끔 아래 사격 | 100 |
| 빠른 적 | GREEN | 1 | 빠른 이동/사격(`formTick`·`attackTick`↓) | 200 |
| 강한 적 | PURPLE | 2~3 | 느린 이동, 탄막(다발) 발사 | 500 |

- 스테이지가 오를수록 빠른 적·강한 적의 비율이 늘고, 편대 속도·공격 빈도가 증가.

### 6.5 총알
- **플레이어 총알(`|`)**: 위로 이동, 화면 상단(`< FORM_TOP-1`) 또는 적 명중 시 `Kill`.
- **적 총알(`!`)**: 아래로 이동, 화면 하단(`> PLAYER_Y+1`) 또는 플레이어 명중 시 `Kill`.

### 6.6 충돌 판정 (풀 × 풀)
- **내 총알 → 적**: 활성 `pBullets` × 활성 `enemies`. 겹치면 `enemy.hp--`, 총알 `Kill`. `hp<=0`이면 FX 스폰 + 점수 + 적 `Kill`.
- **적 총알 → 나**: 무적 아니면 겹칠 때 피격(6.2), 적 총알 `Kill`.
- **적 → 나**: 무적 아니면 겹칠 때 피격 + 그 적 `Kill`(스펙: "플레이어→적→Life 감소→적 제거").
- 판정은 논리 정수 좌표 일치/AABB(관대하게). 비용 O(활성총알 × 활성적) — 수가 커지면 공간 분할로 최적화(폴리시, §13).

### 6.7 점수 · 생명
- 점수: 기본 `100` / 빠른 `200` / 강한 `500`, **스테이지 클리어 보너스 `+1000`**.
- 생명: 시작 `3`, HUD에 `♥` × 남은 수. `0`이면 GAME OVER. HI 스코어는 세션 유지(파일 저장은 §13/Phase4).

### 6.8 스테이지 · 난이도
| 스테이지 | 적 수 | 편대(예) |
|:---:|:---:|---|
| 1 | 12 | 6 × 2 |
| 2 | 16 | 8 × 2 (또는 4 × 4) |
| 3 | 20 | 5 × 4 |
| N | 증가 | 열/행 확장 |
- 스테이지↑ → 적 수·편대 속도(`formTick`↓)·공격 빈도(`attackTick`↓) 증가, 빠른/강한 적 등장.
- 현재 스테이지 적을 전부 제거하면 `STAGECLEAR`(보너스) → 다음 스테이지 편대 스폰.

### 6.9 폭발 애니메이션
- `fxs.Spawn(x,y)` → `frame`을 `GetTickTimer(0.06f)`마다 증가시켜 `*` → `×` → `+` 순으로 바꾸고 4~6프레임 후 `Kill`.
- (선택) 발사 `Beep(700,15)`, 폭발 `Beep(200,30)` — 아주 짧게. 프레임 끊김 거슬리면 생략(Dino와 동일 주의).

---

## 7. 상태 머신

`IGameContent`의 페이즈(`TITLE/INGAME`)는 공용이라 수정하지 않고, 인게임 세부 상태는 `GalagaContent` 내부 enum으로 관리(기존 게임들과 동일 전략). 스펙의 5상태(Title/Playing/Pause/StageClear/GameOver)를 매핑:

```
IGameContent::_EPhase
 ├─ TITLE   → OnTitleUpdate/Render  (GALAGA 로고 + PRESS ENTER, 별 배경)
 └─ INGAME  → OnInGameUpdate/Render
       └─ GalagaContent::_EGameState
            ├─ PLAYING    : 정상 플레이
            ├─ PAUSE      : 일시정지 오버레이(P)
            ├─ STAGECLEAR : "STAGE CLEAR +1000" 연출 → Enter로 다음 스테이지
            └─ GAMEOVER   : 결과 + 재시작(SPACE/Enter)
```
- `ESC`(인게임) → `currentPhase = TITLE`. 타이틀 `ESC` → `main.cpp`가 종료.
- 재시작/다음 스테이지는 풀 `Clear()` + 편대 재스폰(재할당 없음).

---

## 8. 프레임워크 통합

### 8.1 추가/수정 파일
| 파일 | 작업 |
|------|------|
| `framework.h` | `enum class _ECONTENT`에 `GALAGA` 추가(끝에, 정수 순서 유지) |
| `Main/MainContent.cpp` | `SCENE->AddContent((int)_ECONTENT::GALAGA, new GalagaContent());` 등록 |
| `Main/Content/GalagaContent.h` | 신규 — `IGameContent` 상속 (ASCII 주석 권장) |
| `Main/Content/GalagaContent.cpp` | 신규 — 전각 글리프 포함 시 **CP949 저장** |
| `CSGP.vcxproj` / `.filters` | 신규 두 파일 등록(UTF-8, Edit 가능) |

### 8.2 스펙의 "개발 구조" → CSGP 매핑
스펙은 매니저들을 나열했지만, CSGP에서 게임은 **하나의 `IContent` 씬**이다. 공용 매니저는 프레임워크가 제공하고, 게임 고유 모듈은 **씬 내부의 풀 + 메서드**가 된다:

| 스펙 모듈 | CSGP 실현 |
|-----------|-----------|
| Game | `GalagaContent`(씬) + `main.cpp` 루프 |
| Input | 프레임워크 `INPUT`(InputManager) |
| Renderer | 프레임워크 `SCREEN`(ScreenManager) + `DrawXxx()` 메서드 |
| Player | `px/life/score/...` 멤버 + `UpdatePlayer()` |
| **BulletManager** | **`Pool<Bullet> pBullets/eBullets` + `UpdateBullets()`** |
| **EnemyManager** | **`Pool<Enemy> enemies` + 편대 상태 + `UpdateEnemies()`** |
| Collision | `HandleCollisions()` |
| StageManager | `stage/formation` + `SpawnStage()` |
| ScoreManager | `score/hiScore` |
| UI | `DrawHUD()` |
| Sound | `Beep()`(선택) |
| SaveManager | 하이스코어 파일 저장(§13 / Phase4) |

### 8.3 클래스 설계 (헤더 스케치)
```cpp
class GalagaContent : public IGameContent {
private:
    static const int HUD_Y = 1, FORM_TOP = 3;
    static const int PLAY_LEFT = 1, PLAY_RIGHT = 37, PLAYER_Y = 21;

    static const int MAX_PBULLET = 32;
    static const int MAX_EBULLET = 64;
    static const int MAX_ENEMY   = 40;
    static const int MAX_FX      = 32;

    enum class _EGameState { PLAYING, PAUSE, STAGECLEAR, GAMEOVER };
    enum class _EEnemyType { BASIC, FAST, STRONG };

    struct Bullet { float x, y, vx, vy; };
    struct Enemy  { _EEnemyType type; int col, row, hp; };
    struct Fx     { float x, y; int frame; };

    // --- object pools (BulletManager / EnemyManager / FX) ---
    Pool<Bullet, MAX_PBULLET> pBullets;
    Pool<Bullet, MAX_EBULLET> eBullets;
    Pool<Enemy,  MAX_ENEMY>   enemies;
    Pool<Fx,     MAX_FX>      fxs;

    // player
    float px;
    int   life, score, hiScore;
    bool  invincible; int invTimer;

    // formation
    float formX; int formY, formDir;
    int   formCols, formRows;
    int   stage, aliveCount;

    _EGameState gameState;

public:
    virtual void OnInit();
    virtual void OnRelease();
    virtual void OnTitleUpdate();   virtual void OnTitleRender();
    virtual void OnInGameUpdate();  virtual void OnInGameRender();

private:
    void StartGame();               // 스테이지1부터, 모든 풀 Clear
    void SpawnStage(int stage);     // 편대 스폰 (풀에 적 채움)

    void UpdatePlayer();
    void UpdateBullets();           // pBullets + eBullets
    void UpdateEnemies();           // 편대 이동 + 사격
    void UpdateFx();
    void HandleCollisions();

    int  EnemyX(const Enemy& e) const;   // formX + col*SPACING
    int  EnemyY(const Enemy& e) const;

    void DrawPlayer();  void DrawBullets();  void DrawEnemies();
    void DrawFx();      void DrawHUD();
};
```
> `Pool<T,N>`는 헤더 상단(또는 공용 `Pool.h`)에 정의. **메모리 모델**: 모든 풀이 고정 배열(값 타입) — 생성자에서 잡히고 게임 내내 재사용, `OnRelease`는 사실상 비어 있다.

### 8.4 타이머 사용 요약
| 용도 | 방식 | 키(초, 예시) |
|------|------|--------|
| 이동/탄 이동 | 매 프레임 고정 스텝 | — |
| 발사 쿨다운 | `GetTickTimer(fireDelay)` | 0.18 |
| 편대 이동 | `GetTickTimer(formTick)` | 스테이지별 0.5→0.2 |
| 적 사격 | `GetTickTimer(attackTick)` | 스테이지별 0.9→0.4 |
| 폭발 프레임 | `GetTickTimer(0.06f)` | 0.06 |
| 무적 깜빡 | `invTimer` 프레임 카운트 | — |

---

## 9. 렌더링 계획 (OnInGameRender)

1. **배경**(선택): 느리게 흐르는 별 몇 개.
2. **적**: 활성 `enemies`(종류별 색 `▼`).
3. **총알**: 활성 `eBullets`(`!`) → `pBullets`(`|`).
4. **폭발 FX**: 활성 `fxs`(`* × +` 프레임).
5. **플레이어**: `▲`. 무적 중엔 프레임 토글로 깜빡.
6. **HUD**: 상단 y=1 `SCORE / HI / STAGE / LIFE ♥×n`, 하단 y=23 조작 안내, y=22 구분선.
7. **오버레이**: `PAUSE`("-- PAUSE --"), `STAGECLEAR`("STAGE CLEAR  +1000  ENTER"), `GAMEOVER`("GAME OVER" + "PRESS SPACE").

> 성능: 각 풀을 한 번씩 순회(활성만 그림) → 상한 고정이라 최악 비용 예측 가능. 셀 호출이 많아지면 변경분만 그리기/`WriteConsoleOutput`로 최적화(폴리시). MVP는 매 프레임 전면 렌더.

---

## 10. 핵심 데이터 요약

```
pBullets / eBullets (Pool<Bullet>)   : 총알 풀 (x,y,vx,vy)
enemies             (Pool<Enemy>)    : 적 풀 (type,col,row,hp)
fxs                 (Pool<Fx>)       : 폭발 풀 (x,y,frame)
px / life / score / invincible,invTimer : 플레이어
formX,formY,formDir / formCols,formRows / aliveCount : 편대 상태
stage / hiScore                      : 진행 / 최고점
gameState                            : PLAYING/PAUSE/STAGECLEAR/GAMEOVER
```

---

## 11. 구현 마일스톤 (스펙 Phase 매핑)

| Phase | 목표 | 산출물 |
|:---:|------|--------|
| **1** | **기반** | GalagaContent 생성·등록(`_ECONTENT::GALAGA`), 화면/HUD 출력, 플레이어 `A/D` 이동, `SPACE` 발사(`pBullets` **풀** + 쿨다운), 적 편대 스폰(`enemies` 풀) |
| **2** | **전투** | 편대 이동, 내탄 vs 적 충돌 → 격추 + FX(`fxs` 풀), 점수 |
| **3** | **위협** | 적 사격(`eBullets` 풀), 적탄/적기체 vs 나 충돌, 생명·무적, GAME OVER/재시작 |
| **4** | **완성** | 스테이지 진행(12/16/20…)·난이도 증가(속도/빈도/종류), 폭발 애니메이션 다듬기, 하이스코어 저장, **프리 리스트 O(1) 최적화** |

**권장 순서**: Phase 1~3이 "플레이 가능한 최소 갤러그"(쏘고 맞히고 맞으면 죽음). **오브젝트 풀은 Phase 1에서 도입해 Phase 2~3로 확장**, Phase 4에서 프리 리스트로 최적화하며 "측정 후 개선"을 체험. Phase마다 빌드(`MSBuild CSGP.sln /p:Configuration=Debug /p:Platform=x64`)로 검증.

### MVP 목표(스펙)
플레이어 좌우 이동 · 총알 발사 · 적 편대 이동 · 적 제거 · 점수 · 생명 · 스테이지 진행 · 게임 오버 · 콘솔 60FPS 실시간. — 위 Phase 1~4로 충족.

---

## 12. 인코딩 · 주의사항 (CLAUDE.md / handover.md 연동)

- 신규 `.cpp`에 전각 글리프·한글 문자열을 넣으면 **반드시 CP949로 저장**(기존 소스 규칙). UTF-8 저장 시 콘솔 출력이 깨진다([CLAUDE.md](../CLAUDE.md) 인코딩 섹션). 신규 소스는 **UTF-8 작성 → PowerShell로 CP949 재인코딩 + 글리프 왕복 검증** 권장(Dino/Maze가 이 방식).
- **글리프 검증**: 검증된 CP949 글리프는 `■ ● ◆ ★ ▲ ♥`. `▼`(U+25BC)·`×`(U+00D7)는 **구현 시 왕복 검증** 후 사용, 실패하면 폴백(`▼`→`◆`, `×`→`x`). `| ! * +`는 ASCII(안전, 단각).
- **전각/단각 정렬**: 전각은 `logicalX*2`에 2칸, 단각(`| !` 등)은 같은 위치 1칸. 충돌·로직은 **논리 정수 좌표**로만. (§3)
- 헤더는 ASCII(영문 주석) 권장. `framework.h`의 `_ECONTENT`에 `GALAGA`를 **끝에 추가**(정수 순서 유지).
- **`Windows.h` 이름 충돌 주의**: `RED/GREEN/BLUE`는 framework.h 색상수, `IN/OUT/NEAR/FAR/small` 등. 구조체/enum은 `Bullet/Enemy/_E...`처럼 안전하게. `Pool` 이름 충돌 없음.
- **풀은 반드시 고정 배열**. 재시작/스테이지 전환은 `Clear()`로만, 재할당 금지 — 이 게임의 취지.

---

## 13. 향후 확장 (Out of Scope for MVP)

- **갤러그다운 다이브 공격**: 편대 이탈 → 곡선 궤적 하강 후 복귀(절대 좌표 전환). 트랙터 빔/포획 기믹.
- **보스전**: HP 게이지·다단 패턴.
- **프리 리스트 / 세대 인덱스 풀**: O(1) 스폰 + 안전한 핸들(dangling 방지) — Object Pool 심화.
- **공간 분할 충돌**: 균일 격자로 O(총알×적) 절감.
- **연출**: 화면 흔들림, 콤보/배수, 파워업(연사/확산).
- **하이스코어 파일 저장**([CURRICULUM.md](CURRICULUM.md) §10 "저장/불러오기" 실습과 연계 — 스펙의 SaveManager).

---

*본 GDD는 CSGP 프레임워크(v0.2) 기준. 구현 착수 시 Phase 1 기반부터 진행 권장.*
