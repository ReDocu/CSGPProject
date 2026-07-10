# SnakeContent — Game Design Document (Console Snake)

> CSGP(Console Game Pack) 프레임워크 기반 **콘솔 스네이크(SNAKE)**. 자라나는 뱀 · 아이템 · 트랩 · 터널 워프.
> 커리큘럼 **1단계(자료구조·충돌·상태머신)** 의 결과물이며, 이후 모든 게임이 재사용하는 `IGameContent` 패턴의 최초 사례다.
> 함께 볼 문서: [`CURRICULUM.md`](CURRICULUM.md), [`handover.md`](../handover.md), [`CLAUDE.md`](../CLAUDE.md).
>
> ⚠️ **이 문서는 as-built(구현 후 역설계) GDD** 다. 다른 GDD(Tetris/Dino/…)가 "설계 후 구현"이라면, 이 문서는 **이미 구현된 `SnakeContent.*` 코드를 문서화**한 것이다. §11에 현재 구현 상태와 개선 후보, §12에 알려진 이슈를 정리한다.

---

## 0. 이 게임의 설계 의도 (커리큘럼: **자료구조 · 충돌 · 상태머신**)

스네이크는 커리큘럼의 **출발점**이다. "플레이어의 입력이 자료구조를 바꾸고 → 충돌이 규칙을 만드는" 상호작용의 최소 완성형을 만든다.

- **동적 자료구조 + 포인터/소유권** — `std::vector<Snake*>` 로 몸통을 관리하고 `new/delete` 로 마디를 늘린다. 힙 메모리를 처음 직접 다룬다.
- **충돌 판정** — 머리 vs (벽 / 자기 몸 / 아이템 / 트랩 / 터널) 좌표 비교가 곧 게임 규칙.
- **방향 입력과 무효 입력 차단** — `_EDIRECT` + `lastDirect` 로 역주행 금지.
- **상태머신** — 0단계의 `_EPhase(TITLE/INGAME)` + 게임 고유 서브 상태(`_ESELECT` 메뉴).

> 이후 단계는 여기서 쓴 "값 하나짜리 오브젝트를 리스트로 관리 + 충돌 + 상태머신"을 **값 타입 벡터 → 오브젝트 풀**로 발전시킨다(Dino/Road Fighter → Galaga/Battle City).

---

## 1. 개요

| 항목 | 내용 |
|------|------|
| 제목 | SNAKE (CSGP 콘텐츠, 시작 씬) |
| 장르 | 아케이드 (실시간) |
| 플랫폼 | Windows 콘솔 (Win32 API) |
| 플레이 인원 | 1인 |
| 씬 클래스 | `SnakeContent : public IGameContent` |
| 씬 enum | `_ECONTENT::SNAKE` |
| 목표 | 뱀을 조작해 아이템(★)을 먹어 몸을 늘리고 점수 획득. 벽·자기 몸·트랩 충돌 시 생명 감소, 생명 0이면 타이틀로 |

---

## 2. 핵심 게임 루프

```
입력(방향 전환/역주행 차단) → [이동 틱 0.1s] 몸통 팔로우 이동 → 머리 이동
   → 충돌 검사(아이템 / 트랩 / 터널 / 자기 몸 / 벽) → 성장·점수·생명 처리
   → 화면 출력 → 다음 프레임
```

프레임워크 고정 루프(~60fps) 위에서 **뱀 이동만 `TIMER->GetTickTimer(0.1f)`(초당 10칸)** 로 제어한다. 입력은 매 프레임 받되, 실제 위치 변화는 이동 틱에만 일어난다.

---

## 3. 화면 구성 & 좌표 매핑

논리 필드는 **`WALL_X × WALL_Y = 25 × 25`** (Snake 자체 상수, `GAME_SIZE 40×25` 와 별개). 렌더 시 x는 `×2`(전각 1칸=콘솔 2칸). 우측에 UI 패널.

```
 x:0        전장 (내부 플레이 2~22)          25(콘솔 50~) UI 패널
 ┌──────────────────────────┬──────────────────────┐
 │ ■■■■■■■■■■■■■■■■■■ (벽)      │  S N A K E   G A M E  │
 │ ■                        ■ │  SCORE :  120         │
 │ ■   ★   ♠      □          ■ │  STAGE :  1           │
 │ ■        ■(head)          ■ │  LIFE  :  5           │
 │ ■     ■■■(body)  □         ■ │                       │
 │ ■           ♠      ★       ■ │  GAME TIME : 37       │
 │ ■■■■■■■■■■■■■■■■■■ (벽)      │  PLAY TIME : 210      │
 └──────────────────────────┴──────────────────────┘
```

### 배치 상수
| 상수 | 값 | 의미 |
|------|----|------|
| `WALL_X` / `WALL_Y` | 25 / 25 | 전장 크기 |
| `wallPosX` / `wallPosY` | 1 / 1 | 벽 오프셋(경계 두께) |
| 벽 경계 | x∈{1,23}, y∈{1,23} | 그려지는 벽. 내부 플레이 = **2~22** |
| `SPAWN_X` / `SPAWN_Y` | 5 / 5 | 뱀 머리 스폰 |
| UI 패널 | 콘솔 x = `WALL_X*2` = 50 | 우측 정보창 |

- 오브젝트 좌표는 **정수 타일**. 렌더 시 `xPos * 2`.
- 아이템/트랩/터널 스폰 범위 = `2 + rand()%20` → **2~21**.

---

## 4. 조작 (InputManager)

| 페이즈 | 키 | 동작 | 입력 방식 |
|--------|----|------|-----------|
| 타이틀 | `↑ / ↓` | 메뉴 커서(게임시작 ▶ 게임종료) | `OnKeyDown` |
| 타이틀 | `Enter` | 선택 확정(시작→인게임 / 종료→프레임워크 타이틀 씬) | `OnKeyDown` |
| 인게임 | `← → ↑ ↓` | 진행 방향 전환(**역주행 입력 무시**, 길이 1일 땐 허용) | `OnKeyDown` |
| 인게임 | `SPACE` | 타이틀로 복귀 | `OnKeyDown` |

> ⚠️ **뒤로가기 규칙 차이**: Snake는 **`SPACE`로 타이틀 복귀**를 쓴다. 이후 게임들의 "인게임 ESC → 타이틀" 규칙([handover.md](../handover.md) §8)은 Snake 이후 도입됐다. Snake는 인게임에서 ESC를 소비하지 않으므로 인게임 ESC는 `main.cpp`로 전달되어 **프로그램이 종료**된다. (§12 참고)

---

## 5. 엔티티 / 글리프 정의 (실제 구현값)

| 요소 | 글리프 | 색(framework.h) | 비고 |
|------|--------|------------------|------|
| 뱀 머리 | `■`(U+25A0) | `GREEN`(10) | 리스트 head |
| 뱀 몸통 | `■`(U+25A0) | `DARKGREEN`(2) | 성장 시 tail 위치에 추가 |
| 아이템 | `★`(U+2605) | `RED`(12) | 10개, 먹으면 재배치 |
| 트랩 | `♠`(U+2660) | `DARKBLUE`(1) | 10개, 밟으면 생명-1 |
| 터널(쌍) | `□`(U+25A1) | `9 + i` (쌍마다 다른 색) | 6쌍, start↔end 워프 |
| 벽 | `■`(U+25A0) | `DARKGRAY`(8) | 경계, 밟으면 생명-1 |
| 타이틀 로고 | `■` 아트 | 기본색 | "SNAKE" 5줄 블록 아트 |

> 모든 전각 글리프는 CP949 검증 세트(`■ ● ◆ ★ ▲ ♥`) + `♠ □` 로, 소스가 CP949 바이트를 그대로 `WriteFile` 출력한다.

---

## 6. 게임 시스템

### 6.1 뱀 이동 & 성장
- **이동 틱**: `GetTickTimer(0.1f)` (초당 10칸).
- **몸통 팔로우**: head부터 순회하며 각 마디가 **앞 마디의 직전 위치**를 물려받는다(고전 스네이크 방식). 그 뒤 `MoveSnake()` 가 head를 `direct` 방향으로 1칸 옮긴다.
- **역주행 차단**: `lastDirect` 의 반대 방향 입력은 무시(단, 길이 1이면 허용). 이동 후 `lastDirect = direct`.
- **성장**: 아이템 획득 시 `snake.push_back(new Snake(tail.x, tail.y, "■", DARKGREEN))` — tail 위치에 1마디 추가.

### 6.2 아이템 (`itemList`, 10개)
- 머리가 아이템 칸에 진입 → `MoveItem()`(재배치) + 몸 +1 + **score += 10**.
- 재배치는 스폰(5,5)·트랩 위치를 피한다.

### 6.3 트랩 (`trapList`, 10개)
- 머리가 트랩 칸에 진입 → `SnakeInit()`(뱀을 길이 1·스폰으로 리셋) + **life-1**.
- 트랩 배치는 스폰·아이템 위치를 피한다.

### 6.4 터널 워프 (`tunnelList`, 6쌍)
- 각 `TunnelConnect` 는 `startTunnel` / `endTunnel` 한 쌍. 머리가 한 쪽에 진입하면 **반대쪽으로 순간이동** 후 `MoveSnake()` 한 번(빠져나감).
- 쌍마다 색이 다르다(`9 + i`)—어느 입구가 어느 출구인지 색으로 구분.

### 6.5 충돌 판정
| 대상 | 결과 |
|------|------|
| 아이템 `★` | 성장 + 점수 +10, 아이템 재배치 |
| 트랩 `♠` | 생명 -1, 뱀 리셋 |
| 터널 `□` | 반대쪽으로 워프 |
| 자기 몸 | 생명 -1, 뱀 리셋 |
| 벽(x/y ≤1 또는 ≥23) | 생명 -1, 뱀 리셋 |

- 판정은 **정수 타일 좌표 일치**(머리 vs 대상). 벽은 좌표 경계 비교.

### 6.6 점수 · 생명 · 스테이지
- **점수**: 아이템당 +10.
- **생명**: 시작 5. `life <= 0` 이면 `currentPhase = TITLE`(내부 타이틀 복귀).
- **스테이지**: HUD에 `STAGE 1` 표시되나 **증가 로직 없음**(항상 1). — 확장 여지(§13).
- **시간**: `GAME TIME`(콘텐츠 경과), `PLAY TIME`(프로그램 경과) 표시.

---

## 7. 상태 머신

```
IGameContent::_EPhase
 ├─ TITLE  → OnTitleUpdate/Render
 │            └─ SnakeContent::_ESELECT { START, EXIT } (↑/↓ 커서, Enter 확정)
 │                 · START → currentPhase = INGAME
 │                 · EXIT  → SCENE->ChangeContent(TITLE)  (프레임워크 타이틀 씬으로)
 └─ INGAME → OnInGameUpdate/Render
              · SPACE → currentPhase = TITLE
              · life<=0 → currentPhase = TITLE
```

---

## 8. 프레임워크 통합

### 8.1 파일 / 등록
| 파일 | 내용 |
|------|------|
| `Main/Content/SnakeContent.h/.cpp` | 씬 구현 (CP949) |
| `framework.h` | `_ECONTENT::SNAKE` |
| `Main/MainContent.cpp` | `AddContent((int)_ECONTENT::SNAKE, new SnakeContent())` |

### 8.2 자료구조 (헤더)
```cpp
class SnakeContent : public IGameContent {
    enum class _ESELECT { NONE, START, EXIT, END };
    enum class _EDIRECT { NONE, RIGHT, LEFT, UP, DOWN };

    class Snake  { int xPos, yPos; std::string shape; unsigned short color; };
    class Item   { /* 동일 필드 */ };
    class Trap   { /* 동일 필드 */ };
    class Tunnel { /* 동일 필드 */ };
    class TunnelConnect { Tunnel* startTunnel; Tunnel* endTunnel; void DestroyTunnel(); };

    std::vector<Snake*>        snake;        std::vector<Snake*>::iterator        snakeIter;
    std::vector<Item*>         itemList;     std::vector<Item*>::iterator         itemListIter;
    std::vector<Trap*>         trapList;     std::vector<Trap*>::iterator         trapListIter;
    std::vector<TunnelConnect*> tunnelList;  std::vector<TunnelConnect*>::iterator tunnelListIter;

    _ESELECT select;  _EDIRECT direct, lastDirect;
    int stage, score, life;  std::string userName;
};
```
- **메모리 모델**: 모든 오브젝트가 **힙 할당(`new`)**, 벡터에 포인터로 보관. `OnRelease()` 와 `SnakeInit/ItemInit/TrapInit/TunnelInit` 에서 `delete`. (오브젝트 풀을 쓰는 Galaga/Battle City와 대조 — 커리큘럼의 "순진한 관리" 사례.)

### 8.3 주요 메서드
| 메서드 | 역할 |
|--------|------|
| `SnakeInit()` | 뱀을 길이 1·스폰(5,5)으로 리셋 |
| `ItemInit(n)` / `TrapInit(n)` / `TunnelInit(n)` | 오브젝트 n개(쌍) 생성 (OnInit에서 10/10/6) |
| `MoveSnake()` | head를 `direct` 방향 1칸 |
| `MoveItem/MoveTrap()` | 겹침 회피 재배치 |

### 8.4 타이머
| 용도 | 방식 |
|------|------|
| 뱀 이동 | `GetTickTimer(0.1f)` (10칸/초) |
| 경과 시간 HUD | `GetContentTime()` / `GetProgramTime()` |

---

## 9. 렌더링 (OnInGameRender)

1. **UI 패널**(콘솔 x=50): 타이틀·SCORE·STAGE·LIFE·GAME TIME·PLAY TIME.
2. **뱀** → **아이템** → **트랩** → **터널** → **벽** 순서로 `OnDrawColor(x*2, y, shape, color)`.
3. 매 프레임 전면 렌더(오브젝트 수가 적어 부담 없음).

---

## 10. 핵심 데이터 요약

```
snake      (vector<Snake*>)        : 뱀 마디 (head=begin, 성장 시 tail 추가)
itemList   (vector<Item*>, 10)     : 아이템 ★
trapList   (vector<Trap*>, 10)     : 트랩 ♠
tunnelList (vector<TunnelConnect*>,6): 터널 워프 쌍 □
direct/lastDirect                  : 진행 방향 / 역주행 차단
score / life / stage               : 점수 / 생명(5) / 스테이지(고정 1)
```

---

## 11. 현재 구현 상태 · 개선 후보

| 영역 | 현재(as-built) | 개선 후보 |
|------|----------------|-----------|
| 이동/성장 | ✅ 팔로우 이동, 아이템 성장 | — |
| 충돌 | ✅ 벽/몸/트랩/아이템/터널 | 머리-트랩/아이템 중복칸 우선순위 정리 |
| 생명/재시작 | ✅ life 5, 0이면 타이틀 | **게임오버 화면·재시작 시 life/score 리셋**(§12 이슈) |
| 스테이지 | ⬜ 표시만, 미증가 | 점수/아이템 소진 기반 스테이지 진행 + 난이도(속도↑, 트랩↑) |
| 저장 | ⬜ 없음 | 하이스코어·userName 파일 저장 |
| 뒤로가기 | SPACE(구식) | ESC 규칙(§8 handover) 통일 검토 |

---

## 12. 인코딩 · 알려진 이슈

- **CP949 저장**: `.cpp`에 전각 글리프(`■ ★ ♠ □`)·한글이 들어있다. 편집은 **PowerShell CP949 보존**(ASCII 앵커) — [CLAUDE.md](../CLAUDE.md) 인코딩 규칙 준수.
- **알려진 이슈(as-built)**:
  1. **반복자를 클래스 멤버로 보관**(`snakeIter` 등) → 중첩 순회 시 상호 간섭 위험. 지역 반복자/범위 for로 리팩터 권장([handover.md](../handover.md) §10).
  2. **재시작 시 상태 미리셋** — `life<=0 → TITLE` 후 내부 메뉴에서 START를 다시 골라도 `life/score` 가 리셋되지 않는다(즉시 타이틀로 튕김). 완전 초기화는 씬을 나갔다 재진입(`OnInit`)해야 함. → 메뉴 START에서 `SnakeInit + life/score 리셋` 추가 권장.
  3. **stage 미진행** — HUD 표시만 있고 증가 로직 없음.
  4. **ESC 처리 부재** — 인게임 ESC가 프로그램 종료로 이어짐(다른 게임과 규칙 불일치).
  5. **오브젝트 풀 미사용** — `new/delete` 방식. 커리큘럼상 의도된 "순진한 관리"이나, 성능 관점 개선 시 `Pool<T,N>`([Pool.h](../Main/Content/Pool.h)) 적용 가능.

---

## 13. 향후 확장 (Out of Scope)

- **스테이지·난이도 진행**: 목표 점수/아이템 소진 시 다음 스테이지, 속도↑·트랩↑·맵 변화.
- **하이스코어 저장**([CURRICULUM.md](CURRICULUM.md) §12 "저장/불러오기").
- **아이템 다양화**: 감속/가속/보너스/역방향 등 특수 아이템.
- **뒤로가기 규칙 통일**(ESC), **게임오버 연출**.
- **리팩터**: 멤버 반복자 제거, 값 타입/풀 기반 오브젝트 관리.

---

*본 GDD는 구현된 `SnakeContent.*`(커리큘럼 1단계)를 문서화한 as-built 명세다. 개선 착수 시 §11 표와 §12 이슈부터 참고.*
