# MazeContent — Game Design Document

> CSGP(Console Game Pack) 프레임워크 기반 **알고리즘 미로(ALGO MAZE)**.
> `SnakeContent`/`TetrisContent`/`DinoContent`와 동일 패턴(`IGameContent` 상속, `x*2` 좌표, `GetTickTimer`, 색 상수)을 따른다.
> 함께 볼 문서: [`CURRICULUM.md`](CURRICULUM.md), [`handover.md`](../handover.md), [`CLAUDE.md`](../CLAUDE.md), [`TetrisContent_GDD.md`](TetrisContent_GDD.md).
> 이 문서는 **설계 명세**이며 아직 코드는 없다. 구현 순서는 §11 마일스톤 참고.

---

## 0. 이 게임의 설계 의도 (커리큘럼상 위치)

[`CURRICULUM.md`](CURRICULUM.md) 3단계(Tetris)에서 "자료구조 위의 알고리즘"을 배웠다면, 이 게임은 그 **알고리즘을 주제 그 자체로** 끌어올린 심화 단계다.

**핵심 설계 결정 — "미로찾기 알고리즘에 따라 스테이지를 구분한다"의 해석:**

미로에는 두 종류의 알고리즘이 관여한다.
1. **미로 생성(generation)** — 미로를 *어떻게 만드는가* (DFS 백트래커, Prim, Kruskal, Recursive Division, Binary Tree …). 알고리즘마다 미로의 **생김새(시그니처)** 가 확연히 다르다.
2. **미로 풀이/경로탐색(pathfinding)** — 미로를 *어떻게 푸는가* (BFS, DFS, A\*, 벽 따라가기 …).

이 GDD는 **두 계열을 모두** 담되 역할을 나눈다:

- **스테이지 = 생성 알고리즘.** 각 스테이지는 서로 다른 생성 알고리즘으로 미로를 만들어, **미로의 성격 자체가 스테이지마다 달라진다.** (스테이지를 "구분"하는 축)
- **미로찾기(경로탐색) = 공통 학습 메커닉.** 어느 스테이지에서든 `HINT`/`SOLVE` 기능으로 BFS·DFS·A\*·벽 따라가기를 **시각화**해 비교 학습한다. (두 번째 알고리즘 계열을 가르치는 축)

> 결과적으로 이 게임 하나로 **생성 5종 + 탐색 4종**의 알고리즘을 눈으로 보고 손으로 겪는다. 만약 "스테이지 = 풀이 알고리즘"만을 의도했다면 §6.3의 풀이 알고리즘을 스테이지 축으로 바꾸면 되지만, 그 경우 미로 외형이 그대로라 게임의 시각적 다양성이 줄어든다. 그래서 **생성=스테이지 / 탐색=공통 메커닉**을 기본안으로 채택한다.

---

## 1. 개요

| 항목 | 내용 |
|------|------|
| 제목 | ALGO MAZE (CSGP 콘텐츠) |
| 장르 | 알고리즘 퍼즐 / 미로 탐험 |
| 플랫폼 | Windows 콘솔 (Win32 API) |
| 플레이 인원 | 1인 |
| 씬 클래스 | `MazeContent : public IGameContent` |
| 씬 enum | `_ECONTENT::MAZE` (framework.h에 추가) |
| 목표 | 스테이지마다 다른 **생성 알고리즘**으로 만들어진 미로를 시작점(S)에서 출구(E)까지 최단에 가깝게 통과. 걸음 효율로 별점 획득 |

핵심 재미: **"알고리즘마다 미로가 이렇게 다르구나"** 를 몸으로 체감하는 것 + 막히면 **풀이 알고리즘이 미로를 푸는 과정을 눈으로 보는** 학습적 쾌감. MVP는 5종 생성 알고리즘 스테이지 + BFS 기반 최단경로 채점을 목표로 한다.

---

## 2. 핵심 게임 루프

```
스테이지 시작 → (선택) 생성 알고리즘이 미로를 짓는 과정 애니메이션
   → 플레이어가 S에서 출발, 방향키로 미로 탐색(벽이면 이동 무시)
   → 막히면 HINT(다음 최단 몇 칸) 또는 SOLVE(풀이 알고리즘 시각화)
   → 출구 E 도달 → 걸음수 vs 최단 비교로 별점·점수 → 다음 알고리즘 스테이지
   → 마지막 스테이지 클리어 시 결과 요약 → 타이틀
```

프레임워크 고정 루프(`OnUpdate → OnRender`, ~60fps) 위에서, **이동은 입력 이벤트 기반**, **생성/풀이 애니메이션의 스텝 속도는 `TIMER->GetTickTimer()`** 로 제어한다.

---

## 3. 화면 레이아웃 & 좌표 매핑

논리 해상도 `GAME_SIZE 40×25`. 렌더 시 x는 항상 `×2`(전각 1칸=콘솔 2칸). `MainContent`가 최외곽 테두리를 그리므로 내부 x 1~38, y 1~23 사용.

미로는 **(2·셀+1) 격자 모델**을 쓴다: 셀(방) 사이에 벽이 놓인다. 격자 폭 `GRID_W = CELL_COLS*2+1`, 높이 `GRID_H = CELL_ROWS*2+1` (둘 다 홀수).

```
 논리좌표 기준 배치 (y 아래로 증가)
 x:0        (미로 격자 1~27)                 29        38
 ┌────────────────────────────────────────────────────┐ y=0  (MainContent 테두리)
 │ ■■■■■■■■■■■■■■■■■■■■■■■■■■■   STAGE 2/5           │ y=1
 │ ■S    ■       ■         ■   ALGO:              │
 │ ■ ■■■ ■ ■■■■■ ■ ■■■■■■■ ■   Recursive          │
 │ ■   ■ ■ ■   ■   ■     ■ ■   Backtracker(DFS)   │
 │ ■■■ ■ ■ ■ ■ ■■■■■ ■■■ ■ ■                       │
 │ ■ ● ■   ■ ■     ■   ■   ■   STEPS : 041         │  ● = 플레이어
 │ ■ ■■■■■■■ ■■■■■ ■ ■■■ ■■■   BEST  : 032(최단)   │
 │ ■     ■       ■ ■     ■ ■   ─────────────       │
 │ ■■■■■ ■ ■■■■■■■ ■■■■■ ■ ■   H 힌트  F 풀이       │
 │ ■   ■ ■           ■   ■E★   Tab 알고 전환       │  E★ = 출구
 │ ■■■■■■■■■■■■■■■■■■■■■■■■■■■   R 재생성  P 정지    │ y=21
 │                                                    │ y=22
 └────────────────────────────────────────────────────┘ y=24
```

### 배치 상수 (논리좌표)
| 상수 | 값 | 의미 |
|------|----|------|
| `MAZE_X0` | 1 | 미로 격자 좌상단 논리 x |
| `MAZE_Y0` | 1 | 미로 격자 좌상단 논리 y |
| `CELL_COLS` | 13 | 가로 방(셀) 수 |
| `CELL_ROWS` | 10 | 세로 방(셀) 수 |
| `GRID_W` | 27 | `CELL_COLS*2+1` — 격자 가로(글리프) |
| `GRID_H` | 21 | `CELL_ROWS*2+1` — 격자 세로(글리프) |
| HUD 원점 x | 29 | 미로 우측 정보 패널 (콘솔 58~) |

- 격자칸 `(gx, gy)` → 논리 `(MAZE_X0+gx, MAZE_Y0+gy)` → **콘솔 출력 `((MAZE_X0+gx)*2, MAZE_Y0+gy)`**.
- 방(셀) 중심은 격자 좌표에서 `(2·cx+1, 2·cy+1)` (둘 다 홀수). 벽은 짝수 좌표에 위치.
- 격자 최대 폭 콘솔 x = `27*2+1 = 55`, HUD는 논리 29(콘솔 58)부터 → 80폭 안에 안전히 수납.

> `CELL_COLS/ROWS`는 스테이지별로 소폭 키워 난이도를 올릴 수 있다(§6.4). 단 `GRID_W ≤ 27`, `GRID_H ≤ 23`(내부 영역)을 넘지 않도록 클램프.

---

## 4. 조작 (InputManager)

| 키 | 동작 | 입력 방식 |
|----|------|-----------|
| `← ↑ → ↓` | 한 칸 이동 (벽이면 무시) | `OnKeyDown` (+ 유지 시 `GetTickTimer(0.08f)` 반복 이동) |
| `H` | 힌트: 현재 위치에서 출구까지 BFS 최단경로의 **다음 N칸**을 잠깐 표시 | `OnKeyDown` |
| `F` | 풀이 시각화 토글: 선택된 탐색 알고리즘이 미로를 푸는 과정 애니메이션 | `OnKeyDown` |
| `Tab` | 풀이 알고리즘 전환 (BFS→DFS→A\*→벽따라가기 순환) | `OnKeyDown` |
| `G` | 생성 과정 다시 보기(현재 미로를 다시 애니메이션) | `OnKeyDown` |
| `R` | 같은 알고리즘으로 **새 시드** 미로 재생성 | `OnKeyDown` |
| `P` | 일시정지 토글 | `OnKeyDown` |
| `Enter` | 타이틀 확정 / CLEAR 후 다음 스테이지 진행 | `OnKeyDown` |
| `↑ / ↓` | 타이틀 메뉴 이동 | `OnKeyDown` |
| `ESC` | 인게임 → 타이틀, 타이틀 → 종료 | `OnKeyDown` — Tetris/Dino와 동일 규칙 |

> 이동은 **엣지 트리거**(`OnKeyDown`)로 한 번 누름당 한 칸. 길게 누르면 이동 반복 타이머로 연속 이동(퍼즐 조작감). 풀이/생성 시각화 중(`SOLVING`/`GENERATING`)에는 이동 입력을 잠근다.
> ESC 뒤로가기 흐름(인게임→타이틀, 타이틀→종료)은 `main.cpp`가 `OnUpdate` 뒤에서 ESC를 검사하는 기존 구조를 그대로 활용한다([handover.md](../handover.md) §8).

---

## 5. 엔티티/글리프 정의

미로는 정적 격자이므로 "엔티티"보다는 **셀 상태 → 글리프/색 매핑**이 핵심이다.

| 요소 | 글리프 | 색(framework.h) | 비고 |
|------|--------|------------------|------|
| 벽(Wall) | `■` (U+25A0) | `DARKGRAY`(8) 또는 `DARKBLUE`(1) | 검증된 CP949 글리프 |
| 통로(Floor) | 공백 `"  "` | (배경 유지) | 빈 칸은 배경색 |
| 시작점(Start) | `◆` (U+25C6) | `GREEN`(10) | 셀 좌표 `(sx,sy)` |
| 출구(Exit) | `★` (U+2605) | `RED`(12)/`YELLOW`(14) | 셀 좌표 `(ex,ey)` |
| 플레이어(Player) | `●` (U+25CF) | `YELLOW`(14) | 현재 셀 `(px,py)` |
| 방문 흔적(Breadcrumb, 선택) | `·` | `DARKGRAY`(8) | 지나온 길 표시 |
| 탐색 방문(Visited) | `■`/`·` | `DARKSKYBLUE`(3) | 풀이 시각화: 확장된 셀 |
| 탐색 프론티어(Frontier/Queue) | `●` | `SKYBLUE`(11) | 풀이 시각화: 대기 셀 |
| 최종 경로(Path) | `●` | `PURPLE`(13)/`YELLOW`(14) | 풀이 결과 경로 |

> 각 격자칸은 전각 1글리프 = 콘솔 2칸이라 모든 출력이 `x*2`로 정렬된다. 통로는 `"  "`(공백 2칸)로 지워 배경을 유지.
> 검증된 CP949 전각 글리프만 사용: `■ ● ◆ ★`([CLAUDE.md](../CLAUDE.md) 인코딩 섹션).

---

## 6. 게임 시스템

### 6.1 미로 데이터 모델
```cpp
bool wall[GRID_H][GRID_W];   // true = 벽, false = 통로
int  px, py;                 // 플레이어 격자좌표(홀,홀)
int  sx, sy, ex, ey;         // 시작/출구 격자좌표
```
- 초기화: 모든 칸을 벽(`true`)으로 채우고, 생성 알고리즘이 셀 사이 벽을 **뚫어(carve)** 통로를 낸다.
- 셀 `(cx,cy)` ↔ 격자 `(2cx+1, 2cy+1)`. 두 인접 셀 사이 벽 제거 = 그 중간 격자칸을 `false`로.
- 시작 `S = (1,1)`(좌상단 방), 출구 `E = (GRID_W-2, GRID_H-2)`(우하단 방) 기본. 스테이지별로 달리해도 됨.
- **생성 알고리즘은 항상 완전연결(perfect maze) 을 보장** → S에서 E까지 경로가 반드시 하나 이상 존재.

### 6.2 미로 생성 알고리즘 — **스테이지 축**

각 스테이지는 하나의 생성 알고리즘에 대응한다. 알고리즘마다 미로의 **시그니처**가 뚜렷이 달라 스테이지를 시각적으로 구분한다. `_EGenAlgo` enum으로 디스패치.

| 스테이지 | 생성 알고리즘 | 자료구조 | 미로 시그니처(시각적 특징) | 배우는 핵심 개념 | 체감 난이도 |
|:---:|---|---|---|---|:---:|
| **1** | **Binary Tree** | 없음(격자 순회) | 우상단 대각선 편향, 맨 위 행·맨 오른쪽 열이 완전히 뚫린 통로 | 가장 단순한 생성, "편향(bias)"이란 무엇인가 | ★☆☆ |
| **2** | **Recursive Backtracker (DFS)** | 스택 + 방문배열 | 길고 구불구불한 "강줄기" 통로, 갈림 적고 막다른 길이 긺 | 스택·백트래킹·(명시적)재귀 | ★★☆ |
| **3** | **Randomized Prim** | 프론티어 리스트 | 짧은 통로 + 잔가지 많은 덤불형, 시작점 중심 방사 | 프론티어 집합·그리디 성장 | ★★☆ |
| **4** | **Kruskal (Union-Find)** | 분리집합 + 간선리스트 | 편향 없이 균일하게 흩어진 통로 | 분리집합(union-find)·MST·사이클 회피 | ★★★ |
| **5** | **Recursive Division** | 재귀(또는 스택) | 방(room)과 벽으로 재귀 분할된 격자형, 큰 공간 + 좁은 문 | 분할 정복(divide & conquer) | ★★★ |

**각 알고리즘 요지 (구현 노트):**

1. **Binary Tree** — 모든 셀을 순회하며 각 셀에서 "북/동"(또는 남/동) 중 하나를 랜덤으로 골라 벽을 뚫는다. 경계 셀은 가능한 방향만. → 상단 행·우측 열이 항상 뚫려 강한 편향. `O(cells)`, 추가 자료구조 없음. **튜토리얼 스테이지.**
2. **Recursive Backtracker(DFS)** — 시작 셀에서 미방문 이웃을 랜덤 선택→벽 뚫고 전진(스택 push), 막히면 스택 pop해 백트래킹. 스택이 빌 때까지. **재귀 대신 명시적 `std::stack`/`vector` 권장**(깊은 재귀 회피). 긴 복도형 미로.
3. **Randomized Prim** — 시작 셀을 미로에 편입, 그 인접 벽들을 프론티어에 추가. 프론티어에서 하나 랜덤 선택 → 그 벽이 "미로 안 셀 ↔ 밖 셀"을 잇는다면 벽 제거 + 밖 셀 편입 + 새 프론티어 추가. 프론티어가 빌 때까지. 잔가지 많은 미로.
4. **Kruskal + Union-Find** — 셀 사이 모든 벽(간선)을 리스트에 담아 셔플. 하나씩 꺼내 양쪽 셀이 **다른 집합**이면 벽 제거 + `Union`, **같은 집합**이면 스킵(사이클 방지). 경로압축 `Find`. 전체가 한 집합이 되면 완성. 균일한 미로 + MST 직접 체험.
5. **Recursive Division** — 통로만 있는 빈 방에서 시작. 영역을 수평 또는 수직 벽으로 나누되 **벽 한 곳에 문(통로)** 을 남긴다. 나뉜 두 하위 영역을 재귀 분할. 영역이 최소 크기면 종료. 방·복도가 섞인 구조.

> **선택 기능 — 생성 과정 애니메이션**: 각 알고리즘의 carve 스텝을 `GetTickTimer`로 한 스텝씩 그리면 "알고리즘이 미로를 짓는 과정"을 그대로 학습 자료로 볼 수 있다(`GENERATING` 상태, `G`키로 재생). MVP에선 즉시 생성 후, 폴리시로 애니메이션 추가.

> **랜덤**: `main.cpp`에서 `srand(time(NULL))` 이미 호출됨 → `rand()` 사용. 재현을 위해 스테이지마다 **시드값을 저장/표시**(`R` 재생성 시 새 시드).

### 6.3 미로찾기(경로탐색) 알고리즘 — **공통 학습 메커닉**

풀이 알고리즘은 스테이지와 무관하게 **어느 미로에서든** 쓸 수 있는 학습·힌트 도구다. `Tab`으로 전환, `F`로 시각화. `_ESolveAlgo` enum.

| 알고리즘 | 자료구조 | 최단 보장 | 시각화 포인트 | 배우는 개념 |
|---|---|:---:|---|---|
| **BFS** | 큐 + prev맵 | **O (무가중 격자)** | 시작점에서 물결처럼 균등 확장 | 너비우선·최단경로 복원 |
| **DFS** | 스택 | X | 한 방향으로 깊이 파고들다 백트랙 | 깊이우선·최단 아님을 BFS와 대비 |
| **A\*** | 우선순위(맨해튼 휴리스틱) | O (허용 휴리스틱) | 출구 방향으로 **편향된** 확장(BFS보다 적게 탐색) | 휴리스틱 탐색·효율 |
| **벽 따라가기(우수법)** | 없음(방향 상태만) | X | 오른손을 벽에 대고 따라감 | 단순연결 미로 성질·무메모리 탐색 |

- **BFS가 기준(canonical).** 채점용 최단 걸음수 `optimalSteps`는 항상 BFS로 계산한다(무가중 격자에서 BFS = 최단).
- **힌트(`H`)**: 현재 위치→출구 BFS를 돌려 최단경로의 다음 `HINT_LEN`(예: 5)칸만 `PURPLE`로 잠깐 점멸. 힌트 사용 횟수 `hintsUsed` 증가(점수 차감).
- **풀이 시각화(`F`)**: `SOLVING` 상태로 전환, 선택 알고리즘을 스텝 애니메이션 —
  - 확장된 셀 = `Visited(DARKSKYBLUE)`, 대기 셀 = `Frontier(SKYBLUE)`, 완료 후 최종 경로 = `Path(YELLOW)`.
  - `GetTickTimer(0.03f)` 정도로 한 스텝씩. 완료 후 아무 키로 `PLAYING` 복귀(플레이어 위치는 그대로).
- **비교 학습**: 같은 미로에서 `Tab`으로 알고리즘을 바꿔 `F`를 다시 보면 **탐색 셀 수/경로 길이 차이**를 눈으로 비교(BFS의 균등 확장 vs A\*의 목표 편향 vs DFS의 헤맴 vs 벽따라가기의 우회).

### 6.4 클리어 · 스코어 · 스테이지 진행
- **클리어 조건**: 플레이어가 출구 셀 `(ex,ey)`에 도달.
- **점수 구성**:
  | 항목 | 계산 |
  |------|------|
  | 기본 클리어 | `+1000` |
  | 효율 보너스 | `round(1000 * optimalSteps / max(steps, optimalSteps))` — 최단으로 갈수록 만점 |
  | 시간 보너스(선택) | `max(0, TIME_BONUS - (int)GetContentTime()*k)` |
  | 힌트 패널티 | `- hintsUsed * HINT_COST`(예: 100) |
  | 풀이(F) 사용 | 해당 스테이지 효율 보너스 무효(스스로 못 푼 것으로 간주) |
- **별점(★1~3)**: 걸음 효율 `optimalSteps / steps` 기준 — `≥0.9 → ★★★`, `≥0.7 → ★★☆`, 그 외 `★☆☆`.
- **스테이지 진행**: 클리어 → `Enter` → 다음 `_EGenAlgo`로. 스테이지가 오를수록 `CELL_COLS/ROWS`를 소폭 증가(격자 상한 내 클램프)해 난이도 상승.
- **엔딩**: 마지막(Recursive Division) 클리어 시 5개 스테이지 총점·평균 별점 요약 → 타이틀.
- **실패 개념 없음(퍼즐)**: 죽지 않는다. 대신 걸음·시간·힌트가 점수에 반영. (원하면 상위 스테이지에 **걸음 제한/제한시간**을 옵션으로 추가 — §13.)

### 6.5 미로 정합성 보장
- 생성 후 반드시 **BFS로 S→E 도달 가능 검증**. 도달 불가(버그)면 재생성. (완전연결 알고리즘이라 이론상 항상 통과하지만, 경계·인덱싱 실수 방지용 안전장치.)
- `optimalSteps`는 이 검증 BFS에서 함께 산출해 캐시.

---

## 7. 상태 머신

`IGameContent`의 페이즈(`TITLE/INGAME`)는 공용이라 수정하지 않고, 인게임 세부 상태는 `MazeContent` 내부 enum으로 관리한다(Tetris/Dino와 동일 전략).

```
IGameContent::_EPhase
 ├─ TITLE   → OnTitleUpdate/Render  (ALGO MAZE 로고 + 5개 알고리즘 목록 미리보기, START/EXIT)
 └─ INGAME  → OnInGameUpdate/Render
       └─ MazeContent::_EGameState
            ├─ GENERATING : 생성 알고리즘 진행 애니메이션(선택; 스킵 가능)
            ├─ PLAYING    : 정상 플레이(이동/힌트)
            ├─ SOLVING    : 풀이 알고리즘 시각화 중(이동 입력 잠금)
            ├─ CLEAR      : 스테이지 클리어 요약(별점/점수) → Enter로 다음 스테이지
            └─ PAUSE      : 일시정지 오버레이(P 토글)
```
- `ESC`(인게임) → `currentPhase = TITLE`. 타이틀 `ESC` → `main.cpp`가 종료(기존 규칙).
- 스테이지 전환은 `StartStage(stage)`로 미로 재생성 + 상태 리셋.

---

## 8. 프레임워크 통합

### 8.1 추가/수정 파일
| 파일 | 작업 |
|------|------|
| `framework.h` | `enum class _ECONTENT`에 `MAZE` 추가(끝에, 정수 순서 유지) |
| `Main/MainContent.cpp` | `SCENE->AddContent((int)_ECONTENT::MAZE, new MazeContent());` 등록 |
| `Main/Content/MazeContent.h` | 신규 — `IGameContent` 상속 (ASCII 주석 권장) |
| `Main/Content/MazeContent.cpp` | 신규 — 전각/한글 포함 시 **CP949 저장** |
| `CSGP.vcxproj` / `.filters` | 신규 두 파일 등록(UTF-8, Edit 가능) |

### 8.2 클래스 설계 (헤더 스케치)
```cpp
class MazeContent : public IGameContent {
private:
    static const int CELL_COLS = 13, CELL_ROWS = 10;
    static const int GRID_W = CELL_COLS * 2 + 1;   // 27
    static const int GRID_H = CELL_ROWS * 2 + 1;   // 21
    static const int MAZE_X0 = 1,  MAZE_Y0 = 1;

    enum class _EGameState { GENERATING, PLAYING, SOLVING, CLEAR, PAUSE };
    enum class _EGenAlgo   { BINARY_TREE, BACKTRACKER, PRIM, KRUSKAL, DIVISION, COUNT };
    enum class _ESolveAlgo { BFS, DFS, ASTAR, WALL_FOLLOWER, COUNT };
    enum class _ESELECT    { NONE, START, EXIT, END };   // 타이틀 메뉴

    bool wall[GRID_H][GRID_W];        // true = 벽
    int  px, py;                      // 플레이어 격자좌표
    int  sx, sy, ex, ey;              // 시작/출구

    int  stage;                       // 0~4 (= _EGenAlgo 인덱스)
    _EGenAlgo   genAlgo;
    _ESolveAlgo solveAlgo;
    _EGameState gameState;
    _ESELECT    select;

    int  steps;                       // 플레이어 걸음수
    int  optimalSteps;                // BFS 최단(채점용)
    int  score, totalScore, hiScore;
    int  hintsUsed;
    unsigned int seed;                // 현재 미로 시드(표시/재생성)

    // 시각화/탐색 보조 버퍼 (GRID_W*GRID_H)
    // int  dist[GRID_H][GRID_W]; int prevX[..]; int prevY[..]; char visit[..];
    // std::vector<...> frontier;   // 알고리즘별 큐/스택/프론티어

    // Kruskal union-find
    // int  ufParent[CELL_COLS*CELL_ROWS];

public:
    virtual void OnInit();            // 첫 스테이지(BINARY_TREE) 생성, 값 초기화
    virtual void OnRelease();         // 값 타입 위주 → 최소
    virtual void OnTitleUpdate();     virtual void OnTitleRender();
    virtual void OnInGameUpdate();    virtual void OnInGameRender();

private:
    void StartStage(int stage);       // 알고리즘 선택 → 생성 → 검증 → 상태 리셋
    void GenerateMaze(_EGenAlgo a);   // 디스패치
    void GenBinaryTree();  void GenBacktracker(); void GenPrim();
    void GenKruskal();     void GenDivision(int x0,int y0,int x1,int y1);
    bool VerifyAndMeasure();          // BFS로 도달성 검증 + optimalSteps 산출

    void MovePlayer(int dx, int dy);  // 벽 검사 후 이동, steps++
    void ShowHint();                  // BFS 다음 N칸 점멸
    void VisualizeSolve(_ESolveAlgo); // 탐색 애니메이션(Visited/Frontier/Path)

    // union-find 헬퍼
    int  UFind(int a);   void UUnion(int a, int b);

    void DrawMaze();     void DrawPlayer();   void DrawEntities();  // S/E
    void DrawSolveOverlay();  void DrawHUD();
    const char* GenAlgoName(_EGenAlgo a);     // HUD 표기용
    const char* SolveAlgoName(_ESolveAlgo a);
};
```
> **메모리 모델**: 미로/버퍼 모두 **고정 배열·값 타입**으로 설계해 동적 할당·수동 delete를 피한다(SnakeContent의 raw 포인터 문제 회피, Tetris/Dino와 동일 기조). 탐색용 큐/프론티어만 `std::vector`. `OnRelease`는 거의 비게 된다.

### 8.3 타이머 사용 요약
| 용도 | 방식 | 키(초) |
|------|------|--------|
| 이동 반복(길게 누름) | `GetTickTimer(0.08f)` | 0.08 |
| 생성 애니메이션 스텝 | `GetTickTimer(0.02f)` | 0.02 |
| 풀이 시각화 스텝 | `GetTickTimer(0.03f)` | 0.03 |
| 힌트 점멸 | `GetTickTimer(0.1f)` | 0.1 |
| 플레이 타임(시간 보너스) | `GetContentTime()` | — |

---

## 9. 렌더링 계획 (OnInGameRender)

1. **미로 격자**: `wall[gy][gx]`가 true면 `■`(DARKGRAY), false면 `"  "`(배경). 매 프레임 전체 렌더(27×21≈567칸 — 콘솔 부담 시 최적화는 폴리시).
2. **S / E**: 시작 `◆`(GREEN), 출구 `★`(RED/YELLOW).
3. **풀이 오버레이**(`SOLVING` 또는 힌트 활성 시): Visited/Frontier/Path 색으로 해당 셀 덮어 그림.
4. **플레이어**: 현재 셀 `●`(YELLOW). (선택) 지나온 길 breadcrumb `·`.
5. **HUD**(x≥29): `STAGE n/5`, `ALGO: <생성 알고리즘명>`, `SOLVE: <탐색 알고리즘명>`, `STEPS`, `BEST(최단)`, `SEED`, `SCORE`, 조작 안내.
6. **오버레이**: `PAUSE`("-- PAUSE --"), `CLEAR`(별점 ★★☆ + 점수 + "ENTER: NEXT"), `GENERATING`("BUILDING… <알고리즘명>").

> 숫자·문자열은 `std::to_string(x).c_str()`로 출력(기존 콘텐츠와 동일). 성능이 문제되면 "변경 셀만 다시 그리기" 또는 `WriteConsoleOutput` 일괄 출력으로 최적화(폴리시). MVP는 매 프레임 전체 렌더.

---

## 10. 핵심 데이터 요약

```
wall[GRID_H][GRID_W]     : bool 미로 격자(true=벽)          — 생성이 쓰고 렌더/이동이 읽음
px, py                    : 플레이어 격자좌표
sx,sy / ex,ey             : 시작 / 출구
stage / genAlgo           : 현재 스테이지 = 생성 알고리즘
solveAlgo                 : 현재 풀이(힌트/시각화) 알고리즘
steps / optimalSteps      : 플레이어 걸음 / BFS 최단(별점·효율)
score/totalScore/hiScore  : 점수
hintsUsed / seed          : 힌트 사용 수 / 미로 시드
gameState                 : GENERATING/PLAYING/SOLVING/CLEAR/PAUSE
select                    : 타이틀 메뉴
(탐색 버퍼) dist/prev/visit + frontier/stack/union-find
```

---

## 11. 구현 마일스톤

| # | 목표 | 산출물 |
|---|------|--------|
| M1 | **스캐폴딩** | MazeContent 생성·등록(`_ECONTENT::MAZE`), 고정(하드코딩) 미로 1개 렌더(벽/통로/S/E), 이동+벽충돌, 출구 도달=CLEAR |
| M2 | **첫 생성 알고리즘** | Recursive Backtracker(DFS)로 랜덤 미로 생성 + 시드, `VerifyAndMeasure`(BFS 도달성 검증 + optimalSteps) |
| M3 | **BFS 풀이 & 채점** | 걸음수 효율 별점·점수, 힌트(`H`, BFS 다음 N칸), HUD(STEPS/BEST/SCORE) |
| M4 | **알고리즘 스테이지** | 나머지 생성 4종(Binary Tree/Prim/Kruskal/Division) + 스테이지 진행 + HUD 알고리즘명 표기 + 스테이지별 크기 스케일 |
| M5 | **탐색 시각화** | 풀이 시각화(`F`) BFS/DFS/A\*/벽따라가기 + `Tab` 전환 + 생성 애니메이션(`G`) |
| M6 | **상태 & 폴리시** | 타이틀(알고리즘 목록)·CLEAR/PAUSE, 엔딩 요약, 시드 표시/재생성(`R`), 렌더 최적화·수치 튜닝 |

**권장 순서**: M1~M3까지가 "플레이 가능한 최소 미로 게임"(랜덤 미로를 걸어서 출구 도달·채점). M4에서 **스테이지=알고리즘** 축이 완성되고, M5에서 학습적 하이라이트(탐색 시각화)가 붙는다. 마일스톤마다 빌드(`MSBuild CSGP.sln /p:Configuration=Debug /p:Platform=x64`)로 검증.

---

## 12. 인코딩 · 주의사항 (CLAUDE.md / handover.md 연동)

- 신규 `.cpp`에 전각 글리프(`"■" "●" "◆" "★"`)·한글 문자열을 넣으면 **반드시 CP949로 저장**(기존 소스 규칙). UTF-8 저장 시 콘솔 출력이 깨진다([CLAUDE.md](../CLAUDE.md) 인코딩 섹션).
- 헤더는 ASCII(영문 주석) 권장. `framework.h`의 `_ECONTENT`에 `MAZE`를 **끝에 추가**(정수 순서 유지).
- 좌표는 **격자 인덱스(`wall[gy][gx]`)와 콘솔 좌표를 혼동하지 않는다.** 게임 로직은 격자좌표, 출력 직전에만 `((MAZE_X0+gx)*2, MAZE_Y0+gy)`.
- **재귀 주의**: Recursive Backtracker/Division을 순수 재귀로 짜면 격자가 커질 때 스택 부담. 명시적 스택 또는 얕은 재귀로.
- **`Windows.h` 이름 충돌 주의**: 흔한 식별자가 매크로/타입과 충돌 가능(예: `RED/GREEN/BLUE`는 framework.h 색상수, `IN/OUT/NEAR/FAR/small` 등). 방향/상태 enum 이름을 `_E...` 접두 규칙으로 안전하게(예: `_EDIR::NORTH`). ([handover.md](../handover.md) §7 사례 참고.)
- 미로 격자 상한: `GRID_W ≤ 27`, `GRID_H ≤ 23`(내부 영역). 스테이지 스케일 시 클램프.

---

## 13. 향후 확장 (Out of Scope for MVP)

- **생성 알고리즘 추가**: Sidewinder, Eller(스트림 생성), Aldous-Broder/Wilson(균등 스패닝 트리) → 스테이지 확장.
- **가중 미로 + Dijkstra**: 지형(진흙/모래) 비용 도입 → BFS 대신 Dijkstra로 최단, A\* 휴리스틱 비교 심화.
- **braided maze**: 막다른 길 일부를 뚫어 루프 생성(완전연결 아님) → 벽 따라가기의 실패 사례 학습.
- **시야 제한(fog) 모드**: 플레이어 주변만 보이는 탐험 모드(반사신경 대신 기억력).
- **타임어택 / 걸음 제한**: 상위 스테이지 도전 규칙(실패 조건 추가).
- **최고 점수·클리어 기록 파일 저장**(세션 간 유지, [CURRICULUM.md](CURRICULUM.md) §10 "저장/불러오기" 실습과 연계).
- **미로 생성 과정 나란히 비교 화면**: 같은 시드로 5개 알고리즘 미로를 축소 병렬 표시.

---

*본 GDD는 CSGP 프레임워크(v0.2) 기준. 구현 착수 시 M1 스캐폴딩부터 진행 권장.*
