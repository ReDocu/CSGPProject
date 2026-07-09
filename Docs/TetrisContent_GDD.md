# TetrisContent — Game Design Document

> CSGP(Console Game Pack) 프레임워크 기반 테트리스. `SnakeContent`와 동일한 패턴(`IGameContent` 상속, 씬 등록, `x*2` 렌더 좌표)을 따른다.
> 이 문서는 **설계 명세**이며, 아직 코드는 없다. 구현 순서는 §11 마일스톤 참고.

---

## 1. 개요

| 항목 | 내용 |
|------|------|
| 제목 | TETRIS (CSGP 콘텐츠) |
| 장르 | 낙하형 퍼즐 |
| 플랫폼 | Windows 콘솔 (Win32 API) |
| 플레이 인원 | 1인 |
| 씬 클래스 | `TetrisContent : public IGameContent` |
| 씬 enum | `_ECONTENT::TETRIS` (framework.h에 추가) |
| 목표 | 떨어지는 7종 테트로미노를 회전·이동해 가로줄을 채워 지우고, 보드가 넘칠 때까지 최대 점수 획득 |

핵심 재미: **라인 클리어의 즉각적 피드백** + **레벨이 오르며 빨라지는 낙하 속도**의 긴장감. MVP는 표준 테트리스 규칙을 충실히 재현하는 것을 목표로 한다.

---

## 2. 핵심 게임 루프

```
피스 스폰 → (이동/회전/소프트드롭 조작) → 중력으로 낙하 → 바닥/블록에 안착
   → 락다운(lock delay) → 보드에 고정 → 가득 찬 라인 검사 → 라인 클리어 & 점수
   → 다음 피스 스폰 ... (스폰 위치가 막히면 GAME OVER)
```

프레임워크의 고정 루프(`OnUpdate → OnRender`, ~60fps) 위에서, **낙하/락/라인클리어 타이밍은 `TIMER->GetTickTimer()`로 제어**한다.

---

## 3. 화면 레이아웃 & 좌표 매핑

논리 해상도 `GAME_SIZE 40×25`, 렌더 시 x좌표는 항상 `×2`(전각 문자 1칸=콘솔 2칸). `MainContent`가 최외곽 테두리(0,0)~(39,24)를 그리므로 콘텐츠는 내부 x 1~38, y 1~23을 사용한다.

```
 콘솔(80×25) — 논리좌표 기준 배치
 x: 0        12          14                      38
 ┌──────────────────────────────────────────────────┐ y=0  (MainContent 테두리)
 │  ┌────────────┐        T E T R I S               │ y=1  ← 보드 상단 프레임
 │  │            │        ┌──────────┐              │
 │  │            │        │  NEXT    │  (다음 3개)   │
 │  │  PLAYFIELD │        └──────────┘              │
 │  │  10 x 20   │        ┌──────────┐              │
 │  │            │        │  HOLD    │              │
 │  │            │        └──────────┘              │
 │  │            │        SCORE : 12300             │
 │  │            │        LEVEL : 3                 │
 │  │            │        LINES : 27                │
 │  │            │        TIME  : 84                │
 │  │            │                                  │
 │  │            │        ← → 이동  ↑/X 회전         │
 │  └────────────┘        ↓ 소프트  SPACE 하드       │ y=22 ← 보드 하단 프레임
 └──────────────────────────────────────────────────┘ y=24
```

### 보드 좌표 상수 (논리좌표)
| 상수 | 값 | 의미 |
|------|----|------|
| `BOARD_W` | 10 | 플레이필드 가로 셀 수 |
| `BOARD_H` | 20 | 플레이필드 세로 셀 수 |
| `ORIGIN_X` | 2 | 보드 (0,0) 셀의 논리 x |
| `ORIGIN_Y` | 2 | 보드 (0,0) 셀의 논리 y |

- 보드 셀 `(col, row)` → 논리 `(ORIGIN_X + col, ORIGIN_Y + row)` → **콘솔 출력 `((ORIGIN_X+col)*2, ORIGIN_Y+row)`**
- 프레임(벽): 논리 x=1과 x=12(=ORIGIN_X+BOARD_W), y=1과 y=22. `DARKGRAY`로 그림.
- UI 패널 원점: 논리 x=14 (콘솔 28)부터. 여유 x=14~38.

---

## 4. 조작 (InputManager)

| 키 | 동작 | 입력 방식 |
|----|------|-----------|
| `← / →` | 좌/우 이동 | `OnKeyDown` (+ 폴리시로 DAS 반복) |
| `↓` | 소프트 드롭(빠른 낙하, 셀당 +1점) | `OnKeyStay` 또는 소프트드롭 타이머 |
| `↑` 또는 `X` | 시계방향(CW) 회전 | `OnKeyDown` |
| `Z` | 반시계방향(CCW) 회전 | `OnKeyDown` |
| `SPACE` | 하드 드롭(즉시 바닥, 셀당 +2점, 즉시 락) | `OnKeyDown` |
| `C` | 홀드(피스 보관/교체, 드롭당 1회) | `OnKeyDown` |
| `P` | 일시정지 토글 | `OnKeyDown` |
| `Enter` | 타이틀 메뉴 확정 | `OnKeyDown` |
| `↑ / ↓` | 타이틀 메뉴 이동 | `OnKeyDown` |
| `ESC` | 게임 종료(전역, main 루프) | — |

> **역주행/즉시 락 주의**: 회전은 벽/블록과 겹치면 무시(월킥 실패 시). `OnKeyDown`은 엣지 트리거라 한 번 누름당 1회 처리되어 회전/이동에 적합하다.

---

## 5. 테트로미노 정의

7종 표준 피스. **회전 상태 4개를 런타임 계산 없이 사전 정의 테이블로 저장**(콘솔 게임에 가장 안전·단순). 각 회전 상태는 4×4 박스 안의 블록 4칸을 `(dx, dy)` 좌표 리스트로 표현.

### 색상 매핑 (framework.h 상수)
| 피스 | 색 상수 | 값 |
|------|---------|----|
| I | `SKYBLUE` | 11 |
| O | `YELLOW` | 14 |
| T | `PURPLE` | 13 |
| S | `GREEN` | 10 |
| Z | `RED` | 12 |
| J | `BLUE` | 9 |
| L | `DARKYELLOW` | 6 |

> 16색 팔레트에 주황이 없어 L은 `DARKYELLOW`로 대체. 셀 렌더 글리프는 기존 스타일에 맞춰 `"■"`(전각) 사용.

### 회전 데이터 예시 (SRS 스폰 방향 기준, ● = 블록)

**I 피스** (rot 0 / rot 1)
```
 rot0          rot1
 . . . .       . . ● .
 ● ● ● ●       . . ● .
 . . . .       . . ● .
 . . . .       . . ● .
```
**O 피스** (회전 불변)
```
 . ● ● .
 . ● ● .
 . . . .
 . . . .
```
**T 피스** (rot 0~3)
```
 rot0     rot1     rot2     rot3
 . ● .    . ● .    . . .    . ● .
 ● ● ●    . ● ●    ● ● ●    ● ● .
 . . .    . ● .    . ● .    . ● .
```
> S, Z, J, L도 동일하게 SRS 표준 4상태를 테이블로 정의한다(구현 시 상수 배열로 하드코딩).

### 데이터 표현 (제안)
```cpp
// [피스종류][회전상태][블록4개] = {dx, dy}
static const int SHAPES[7][4][4][2] = { ... };
// 보드 셀 값: 0 = 빈 칸, 1~7 = 고정된 피스 종류(색 결정)
```

---

## 6. 게임 시스템

### 6.1 스폰 & 7-bag 랜덤
- **7-bag 방식**: I,O,T,S,Z,J,L 7개를 봉지에 넣고 Fisher-Yates(`rand()`)로 섞어 하나씩 꺼냄. 봉지가 비면 다시 채움 → 한 사이클에 각 피스 정확히 1번씩(공정성).
- `main.cpp`에서 이미 `srand(time(NULL))` 호출됨 → `rand()` 그대로 사용.
- 스폰 위치: 보드 상단 중앙 `col = 3, row = 0`. **스폰 즉시 다른 블록과 겹치면 GAME OVER**.
- `nextQueue`에 최소 3개를 미리 채워 NEXT 미리보기 제공.

### 6.2 중력 & 드롭
- **레벨별 낙하 속도 테이블** (초/셀):
  | 레벨 | 0 | 1 | 2 | 3 | 5 | 8 | 10 | 15+ |
  |------|---|---|---|---|---|---|----|----|
  | 속도(s) | 0.80 | 0.72 | 0.63 | 0.55 | 0.39 | 0.22 | 0.13 | 0.05 |
- **구현**: `if (TIMER->GetTickTimer(gravitySpeed[level])) { 한 칸 낙하 시도 }`.
  - `GetTickTimer`는 인자 float가 곧 타이머 키다. 레벨별 속도값이 discrete하므로 map 엔트리는 사용된 속도 종류만큼만 생성 → 누수 없음.
- **소프트 드롭**: `↓` 유지 중에는 별도 `GetTickTimer(0.05f)`로 빠르게 낙하, 셀당 +1점.
- **하드 드롭**: 가능한 최하단까지 즉시 이동(고스트 위치), 낙하 칸수 ×2점, 즉시 락.

### 6.3 회전 & 월킥(Wall Kick)
- 회전 = 회전 상태 인덱스 `(rot + 1) % 4` (CW) 또는 `(rot + 3) % 4` (CCW).
- 회전 후 충돌 시 **SRS 월킥 오프셋 테이블**로 최대 5개 후보 위치를 순서대로 시도. 모두 실패하면 회전 취소.
- MVP 단계에서는 **기본 월킥(±1칸 좌우, ±1칸 위)** 로 단순화 가능. 정식 SRS 킥 테이블은 폴리시 단계에서 적용.

### 6.4 충돌 & 락다운(Lock Delay)
- **충돌 판정**: 피스의 각 블록이 (a)보드 범위 밖 (b)이미 고정된 셀과 겹침 → 불가.
- **락다운**: 아래로 못 내려가는 상태가 되면 `lockTimer`(≈0.5s) 시작. 만료 시점에도 여전히 못 내려가면 보드에 고정.
  - 이동/회전 성공 시 lock delay 리셋(단, 무한 리셋 방지 위해 **최대 15회** 제한 — 폴리시).
  - 하드 드롭은 lock delay 무시하고 즉시 고정.

### 6.5 라인 클리어
1. 고정 직후 보드 전 행 스캔 → 한 행이 모두 채워졌으면 클리어 대상.
2. (선택) `_EGameState::LINECLEAR` 상태로 전환 후 `GetTickTimer(0.05f)`로 깜빡임/붕괴 연출.
3. 채워진 행 제거 후 위쪽 행을 아래로 shift, 상단은 빈 행으로 채움.
4. 점수·라인 카운트 갱신.

### 6.6 점수 · 레벨 · 라인
- **라인 점수** (레벨 배수 적용): 1줄 `100`, 2줄 `300`, 3줄 `500`, 4줄(테트리스) `800` × `(level + 1)`.
- **드롭 점수**: 소프트 셀당 +1, 하드 셀당 +2.
- **레벨업**: 누적 클리어 라인 10줄마다 레벨 +1 → 낙하 속도 상승.
- 표시: `score`, `level`, `lines`, 그리고 `TIMER->GetContentTime()` 기반 플레이 타임.

### 6.7 홀드(Hold)
- `C` 키로 현재 피스를 `holdType`에 보관. 홀드 칸이 비어있으면 다음 피스를 대신 스폰, 차 있으면 보관 피스와 교체.
- **드롭당 1회 제한**(`holdUsed` 플래그, 피스 고정 시 리셋) — 무한 스왑 방지.

### 6.8 고스트 피스(Ghost)
- 현재 피스가 하드 드롭될 착지 위치를 옅은 색(예: `DARKGRAY`)으로 미리 표시 → 조작 편의. 매 프레임 현재 피스 기준 재계산.

---

## 7. 상태 머신

`IGameContent`의 페이즈(`NONE/TITLE/INGAME`)는 **프레임워크 공용**이라 수정하지 않는다. 게임 오버·일시정지 등 인게임 세부 상태는 **`TetrisContent` 내부 상태 enum**으로 관리한다.

```
IGameContent::_EPhase
 ├─ TITLE   → OnTitleUpdate/Render  (START / EXIT 메뉴, SnakeContent 패턴 재사용)
 └─ INGAME  → OnInGameUpdate/Render
       └─ TetrisContent::_EGameState
            ├─ READY      : 시작 카운트다운(선택)
            ├─ PLAYING    : 정상 플레이
            ├─ LINECLEAR  : 라인 삭제 연출 중(입력 잠금)
            ├─ PAUSE      : 일시정지 오버레이
            └─ GAMEOVER   : 결과 표시 → Enter로 타이틀 복귀
```

- `EXIT` 선택 또는 GAMEOVER 후 복귀는 `SCENE->ChangeContent((int)_ECONTENT::TITLE)` (기존 상위 타이틀 씬으로).

---

## 8. 프레임워크 통합

### 8.1 추가/수정 파일
| 파일 | 작업 |
|------|------|
| `framework.h` | `enum class _ECONTENT`에 `TETRIS` 추가 |
| `Main/MainContent.cpp` | `SCENE->AddContent((int)_ECONTENT::TETRIS, new TetrisContent());` 등록. 시작 씬을 TETRIS로 하려면 `ChangeContent` 변경 |
| `Main/Content/TetrisContent.h` | 신규 — `IGameContent` 상속 |
| `Main/Content/TetrisContent.cpp` | 신규 — 구현 |
| `CSGP.vcxproj` / `.filters` | 신규 두 파일 등록 (VS 외부 생성 시 수동 추가 필요) |

### 8.2 클래스 설계 (헤더 스케치)
```cpp
class TetrisContent : public IGameContent {
private:
    static const int BOARD_W = 10, BOARD_H = 20;
    static const int ORIGIN_X = 2,  ORIGIN_Y = 2;

    enum class _EGameState { READY, PLAYING, LINECLEAR, PAUSE, GAMEOVER };
    enum class _ESELECT   { NONE, START, EXIT, END };

    struct Tetromino { int type; int rot; int x, y; };  // type 0~6, 보드좌표 x,y

    int board[BOARD_H][BOARD_W];   // 0=빈칸, 1~7=고정 피스(색)
    Tetromino current;
    int  holdType;   bool holdUsed;
    std::vector<int> bag;          // 7-bag
    std::vector<int> nextQueue;    // 미리보기 큐

    int score, level, lines;
    _EGameState gameState;
    _ESELECT    select;

    // 낙하/락/연출 타이머 키(초)
    // gravitySpeed[level], softDrop=0.05f, lock=0.5f, clearFx=0.05f

public:
    virtual void OnInit();          // 보드 클리어, 첫 피스 스폰, 값 초기화
    virtual void OnRelease();       // (동적 할당 없으면 최소)
    virtual void OnTitleUpdate();   virtual void OnTitleRender();
    virtual void OnInGameUpdate();  virtual void OnInGameRender();

    // 내부 헬퍼
    void SpawnPiece();
    bool CanMove(const Tetromino& p, int dx, int dy, int newRot);
    void LockPiece();
    int  ClearLines();              // 지운 줄 수 반환
    void HardDrop();  void SoftStep();  void Hold();
    void RefillBag();  int  DrawNext();
    Tetromino GetGhost();           // 착지 위치 계산
    void DrawCell(int col, int row, int color);  // (ORIGIN+col)*2, ORIGIN+row
};
```

> **메모리 모델**: `board`, `bag`, `nextQueue`, 피스 모두 값 타입/고정 배열로 설계해 **동적 할당·수동 delete를 피한다**(SnakeContent의 raw 포인터 문제 회피). `OnRelease`는 거의 비게 된다.

### 8.3 타이머 사용 요약
| 용도 | 호출 | 키(초) |
|------|------|--------|
| 중력 낙하 | `GetTickTimer(gravitySpeed[level])` | 레벨별 테이블 |
| 소프트 드롭 | `GetTickTimer(0.05f)` | 0.05 |
| 락 딜레이 | `GetTickTimer(0.5f)` 또는 수동 경과 측정 | 0.5 |
| 라인 클리어 연출 | `GetTickTimer(0.05f)` | 0.05 |
| DAS 반복(폴리시) | `GetTickTimer(0.05f)` | 0.05 |

---

## 9. 렌더링 계획 (OnInGameRender)

1. **보드 프레임**: 좌/우/하단 벽을 `DARKGRAY "■"`로. (상단은 스폰 영역이라 열어두거나 얇게)
2. **고정 블록**: `board[row][col] != 0`인 셀을 해당 색으로. `DrawCell(col,row,color)`.
3. **고스트 피스**: 착지 위치 4칸 `DARKGRAY`.
4. **현재 피스**: 4칸 피스 색.
5. **UI 패널**(x≥14): 제목, NEXT 미리보기(다음 1~3개), HOLD, SCORE/LEVEL/LINES/TIME, 조작 안내.
   - 숫자는 `std::to_string(score).c_str()`로 출력(SnakeContent와 동일).
6. **오버레이**: PAUSE / GAME OVER 시 보드 중앙에 텍스트 박스.

> 성능: 셀 단위 `OnDrawColor` 호출이 많다. 보드는 20×10=200셀 + 프레임. 60fps에서 부담되면 "변경된 셀만 다시 그리기" 또는 `WriteConsoleOutput` 일괄 출력으로 최적화(폴리시). MVP는 매 프레임 전체 렌더로 시작.

---

## 10. 핵심 데이터 구조 요약

```
board[20][10]   : int (0 빈칸 / 1~7 색)     — 고정된 블록들
current         : Tetromino                 — 지금 조작 중인 피스
nextQueue       : int[]  (front=다음 피스)   — NEXT 미리보기 + 스폰 소스
bag             : int[]  (7-bag 셔플 버퍼)
holdType        : int (-1=없음)             — 보관 피스
holdUsed        : bool                       — 드롭당 홀드 1회 제한
score/level/lines : int
gameState       : _EGameState
select          : _ESELECT                   — 타이틀 메뉴
```

---

## 11. 구현 마일스톤

| # | 목표 | 산출물 |
|---|------|--------|
| M1 | **스캐폴딩** | TetrisContent 파일 생성, `_ECONTENT::TETRIS` 추가, MainContent 등록, 보드 프레임 + 빈 보드 렌더 |
| M2 | **낙하 & 고정** | 스폰, 중력 낙하, 좌우 이동, 충돌 판정, 하드 드롭, 바닥 안착 후 board에 고정 |
| M3 | **회전 & 라인** | 회전 테이블 + 월킥(기본), 가득 찬 라인 검사·삭제·shift |
| M4 | **진행 시스템** | 점수/레벨/라인, 레벨별 낙하 속도, 7-bag + NEXT 미리보기 |
| M5 | **편의 기능** | 홀드, 고스트 피스, 락 딜레이, 소프트 드롭, DAS |
| M6 | **상태 & 폴리시** | 타이틀 메뉴, GAME OVER/PAUSE, 라인 클리어 연출, 정식 SRS 킥, 렌더 최적화 |

**권장 순서**: M1~M3까지가 "플레이 가능한 최소 테트리스", M4~M6은 완성도. 각 마일스톤마다 빌드(`MSBuild CSGP.sln`)로 검증.

---

## 12. 인코딩 · 주의사항 (CLAUDE.md 연동)

- **신규 `.cpp`/`.h`에 전각 글리프(`"■"`)·한글 문자열을 넣으면 반드시 CP949로 저장**한다. 이 프로젝트의 기존 소스가 전부 CP949이고, 콘솔 출력(`WriteFile`)이 CP949 바이트를 그대로 내보내기 때문. UTF-8로 저장하면 런타임에 블록이 깨진다.
- 한글이 든 기존 파일 수정은 Edit 도구 금지 → PowerShell CP949 방식 (CLAUDE.md 참조).
- `framework.h`의 `_ECONTENT` enum에 `TETRIS`를 **끝에 추가**(기존 값의 정수 순서 유지).
- 좌표는 항상 논리좌표로 계산하고 **출력 직전에만 `×2`**. 보드 인덱스(`board[row][col]`)와 화면 좌표를 혼동하지 않는다.

---

## 13. 향후 확장 (Out of Scope for MVP)

- T-Spin 인식 및 보너스 점수
- 콤보 / Back-to-Back 보너스
- 최고 점수 저장(파일 I/O)
- 레벨별 배경색/테마
- 2인 대전(가비지 라인) — 프레임워크에 입력 분리 필요

---

*본 GDD는 CSGP 프레임워크(v0.2) 기준. 구현 착수 시 M1 스캐폴딩부터 진행 권장.*
