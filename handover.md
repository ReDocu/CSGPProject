# CSGP 프레임워크 인수인계 (handover)

> **CSGP (Console Game Pack)** — C++ Win32 콘솔 게임 개발 프레임워크 + 예제 게임 7종(Snake, Tetris, Dino, Road Fighter, Maze, Galaga, Battle City).
> 이 문서는 프레임워크 구조와 개발 규칙, 그리고 최근 작업 내역·다음 할 일을 인수인계하기 위한 문서다.
> 함께 볼 문서: [`CLAUDE.md`](CLAUDE.md) (작업 규칙), [`Docs/`](Docs) (게임별 GDD), [`README.md`](README.md).

최종 갱신 기준일: 2026-07-10

---

## 0. 가장 먼저 알아야 할 것 (요약)

1. **모든 소스(`.cpp`/`.h`)는 CP949(EUC-KR) 인코딩**이다. 한글 주석·전각 문자(`■ ◆ ★ ▲ ♥`)가 들어있다. **UTF-8 편집기로 저장하면 런타임 출력이 깨진다.** → §6 필독.
2. 빌드는 Visual Studio 솔루션(`CSGP.sln`), 타깃 `Debug|x64`, 결과물 `x64\Debug\CSGP.exe`.
3. 구조는 **매니저 싱글턴 + 씬(Scene) 시스템 + 페이즈 상태머신**. 새 게임은 씬 클래스 하나로 붙인다. → §4, §7.
4. **현재 시작 씬은 `BATTLECITY`** 로 설정돼 있다(`MainContent.cpp`). 최근 만든 게임을 바로 테스트하려고 바꿔둔 것이며, 게임 선택화면이 완성되면 이 줄을 되돌린다. → §7.
5. **직전 세션 작업(프레임워크 픽스 · Snake~Maze · GDD)은 커밋됨(`d50a4a8`).** 이후 추가한 **Galaga · Battle City · 공용 `Pool.h`는 아직 커밋 전(uncommitted)**. 브랜치 `main`. → §9.

---

## 1. 빌드 & 실행

```powershell
# MSBuild 경로 찾기
$msbuild = & "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe" `
    -latest -products * -requires Microsoft.Component.MSBuild `
    -find MSBuild\**\Bin\MSBuild.exe

# 빌드 (Debug|x64)
& $msbuild CSGP.sln /p:Configuration=Debug /p:Platform=x64 /m /v:minimal

# 실행 (반드시 실제 콘솔 창에서 — 더블 버퍼링 스크린 버퍼를 쓰므로 파이프 실행 불가)
.\x64\Debug\CSGP.exe
```

- Windows 전용(Win32 콘솔 API 직접 사용). Debug/Release, x86/x64 구성 존재.
- 조작·플레이는 실제 콘솔에서만 확인 가능. 자동 검증은 "빌드 성공 + 프로세스 즉시 종료 여부"까지만 가능하다.
- 기존 경고 1건: `main.cpp(6)` C4244 (`srand(time(NULL))`의 캐스트). 무해하며 이번 작업과 무관.

---

## 2. 디렉터리 구조

```
CSGPProject
├── main.cpp                진입점: 매니저 초기화 → 게임 루프 → 역순 해제
├── framework.h             공용 헤더: 싱글턴 매크로, 화면/색상 상수, _ECONTENT 씬 enum
├── CSGP.sln / .vcxproj     VS 솔루션/프로젝트 (새 파일 추가 시 .vcxproj + .filters 등록 필요)
│
├── FrameWork/              [엔진 계층] 여러 게임이 공유. 특정 게임 로직을 넣지 말 것
│   ├── singleton.h         템플릿 싱글턴 베이스
│   ├── ScreenManager.*     콘솔 더블 버퍼링 렌더링
│   ├── TimerManager.*      틱 타이머 / 프레임 제한
│   ├── InputManager.*      GetAsyncKeyState 기반 키 입력(Down/Up/Stay/Toggle)
│   ├── SceneManager.*      map<int, IContent*> 씬 등록·전환 (씬 소유권 보유)
│   └── Interface/IContent.h  씬 인터페이스 (OnInit/OnRelease/OnUpdate/OnRender + 가상 소멸자)
│
├── Main/                   [콘텐츠 계층] 실제 게임/화면
│   ├── MainContent.*       씬 등록·시작 씬 선택, 현재 씬에 위임, 최외곽 테두리
│   └── Content/
│       ├── Interface/IGameContent.*   IContent + TITLE/INGAME 페이즈 상태머신
│       ├── IntroContent.*  (미완성 스텁)
│       ├── TitleContent.*  프레임워크 타이틀 — 사용자가 "게임 선택화면"으로 구현 예정
│       ├── LoadContent.*   로딩 연출 씬
│       ├── Pool.h          공용 제네릭 오브젝트 풀 Pool<T,N> (Galaga·BattleCity 공유)
│       ├── SnakeContent.*  스네이크 게임
│       ├── TetrisContent.* 테트리스 게임
│       ├── DinoContent.*   크롬 디노 러너
│       ├── RoadFighterContent.*  로드 파이터 레이싱
│       ├── MazeContent.*   미로 — 절차적 맵 생성 + 경로탐색 학습
│       ├── GalagaContent.* 갤러그 — 오브젝트 풀링 학습
│       └── BattleCityContent.*  배틀 시티 — 맵생성 + 풀링 캡스톤
│
├── Docs/                   게임별 설계 문서(GDD) + 학습 커리큘럼
│   ├── CURRICULUM.md       콘솔 게임 7종으로 배우는 개발 기술 지도
│   ├── SnakeContent_GDD.md (as-built — 구현된 코드 역설계)
│   ├── TetrisContent_GDD.md
│   ├── DinoContent_GDD.md
│   ├── RoadFighterContent_GDD.md
│   ├── MazeContent_GDD.md
│   ├── GalagaContent_GDD.md
│   ├── BattleCityContent_GDD.md
│   ├── PacManContent_GDD.md 8단계 팩맨(AI·FSM) — FUTURE_GAMES에서 승격
│   └── FUTURE_GAMES.md     8단계+ 게임 백업(Breakout/Platformer/Sokoban)
├── CLAUDE.md               AI 어시스턴트/개발 규칙 (인코딩·아키텍처 규칙 포함)
└── handover.md             (이 문서)
```

---

## 3. 프레임워크 매니저

`framework.h`에서 싱글턴 접근 매크로를 제공한다:

| 매크로 | 클래스 | 역할 |
|--------|--------|------|
| `SCREEN` | ScreenManager | 콘솔 화면 버퍼 2개로 더블 버퍼링, 문자/색 출력 |
| `TIMER` | TimerManager | 프로그램/콘텐츠 경과시간, 틱 타이머, 프레임 제한 |
| `INPUT` | InputManager | 키 입력 폴링(엣지/레벨 구분) |
| `SCENE` | SceneManager | 씬 등록·전환, 현재 씬 보관 |

### ScreenManager
- **핵심: 전각 문자 1칸 = 콘솔 2칸** 이므로 셀 출력 시 x는 `x*2`.
- `OnDraw(x, y, msg)` / `OnDrawColor(x, y, msg, color, bgColor=255)` — **첫 인자 x는 콘솔 열 좌표**. 텍스트는 콘솔 x 그대로, 셀은 `논리x*2`를 넘긴다.
- `ClearBuffer()` — 문자 + 색 속성 모두 초기화. `FlippingBuffer()` — 백버퍼 활성화 + 인덱스 토글.

### TimerManager
- `GetTickTimer(float sec)` — 지정 간격마다 `true`. **인자값이 곧 타이머의 키**이므로, 가변 간격은 discrete한 값 테이블로 관리한다(예: 스테이지별 스폰 간격, 레벨별 낙하속도).
- `GetContentTime()` / `GetProgramTime()` — 초 단위 경과시간(float). `StartContent()`로 콘텐츠 시간 리셋(게임 시작 시 호출).
- `SetFrame(float ms)` — 남은 시간만큼 `Sleep`. `GetTickCount64` 해상도 10~16ms.

### InputManager
- `OnKeyDown(key)` — **엣지 트리거**. 한 번 눌림당 한 번만 `true`. **한 프레임에 같은 키를 두 번 호출하면 두 번째는 false**(엣지 소비됨) — ESC 뒤로가기 로직이 이 특성을 활용한다(§8).
- `OnKeyUp` — 뗀 순간, `OnKeyStay` — 눌려 있는 동안(레벨), `OnToggleKey` — 토글 비트.

### SceneManager
- `AddContent(key, IContent*)` — 씬 등록(**소유권을 가져감**, 외부 delete 금지).
- `ChangeContent(key)` — 이전 씬 `OnRelease()` → 새 씬 `OnInit()`.
- `OnRelease()` — 현재 씬 해제 + 등록된 모든 씬 delete.

---

## 4. 씬(Scene) 시스템 & 생명주기

메인 루프(`main.cpp`, 약 60fps 고정):
```
ClearBuffer → main->OnUpdate() → [ESC 종료 체크] → main->OnRender() → FlippingBuffer → SetFrame
```
`MainContent`는 현재 씬(`SCENE->GetContent()`)에 Update/Render를 위임하고, 그 뒤 최외곽 테두리를 그린다.

- 씬 인터페이스는 `IContent`(단순 씬) 또는 `IGameContent`(TITLE/INGAME 페이즈가 있는 게임).
- `IGameContent`는 `currentPhase`(NONE/TITLE/INGAME)에 따라 `OnTitleUpdate/Render` 또는 `OnInGameUpdate/Render`로 분기.
- **초기화는 생성자가 아니라 `OnInit()`에서** 한다(씬 재진입 시 다시 불리는 곳은 OnInit).
- `OnRelease()`는 두 번 불려도 안전해야 한다(씬 전환 시 + 프로그램 종료 시).

---

## 5. 좌표계 · 렌더링 규칙

- 논리 해상도 `GAME_SIZE 40×25`(framework.h). 콘솔 스크린 버퍼 80×50.
- **전각 문자(■ 등) 1칸 = 콘솔 2칸** → 게임 로직은 논리 좌표로만 계산하고, **출력 직전에만 `x*2`** 로 변환.
- 색은 `framework.h`의 상수(0~15: BLACK~WHITE)만 사용.
- 매 프레임 전면 렌더가 기본(오브젝트 수가 적어 부담 없음). 커지면 변경분만 그리기/`WriteConsoleOutput` 고려.

---

## 6. ⚠️ 인코딩 규칙 (매우 중요)

**모든 기존 `.cpp`/`.h`는 CP949로 저장되어 있다.** 한글 주석과 전각 글리프가 들어 있고, 콘솔 출력(`WriteFile`)이 이 바이트를 그대로 내보내기 때문에 인코딩이 틀어지면 화면이 깨진다.

- 툴/에디터로 한글이 든 파일을 **UTF-8로 저장하면 손상**된다. (AI 어시스턴트의 Edit/Write 도구는 UTF-8이라 한글 파일에 사용 금지.)
- 한글 포함 파일 수정은 **PowerShell로 CP949 인코딩을 보존**하며 한다. ASCII 부분만 앵커로 잡아 치환:
  ```powershell
  [System.Text.Encoding]::RegisterProvider([System.Text.CodePagesEncodingProvider]::Instance)
  $cp949 = [System.Text.Encoding]::GetEncoding(949)
  $t = [System.IO.File]::ReadAllText($path, $cp949)
  $t = $t.Replace($oldAsciiAnchor, $newAsciiText)   # 한글 라인은 건드리지 않기
  [System.IO.File]::WriteAllText($path, $t, $cp949)
  ```
  - ⚠️ `.Replace()`는 **모든 일치**를 바꾼다. 주석 속 예시 코드까지 바뀔 수 있으니, 유일성이 필요하면 앞뒤 문맥(예: 뒤따르는 `}`)을 포함해 앵커를 유일하게 만든다. (실제로 이 세션에서 시작 씬 변경 시 주석 예시까지 바뀐 적 있음.)
- **신규 소스** 작성: Write 도구로 UTF-8 작성 후 **PowerShell로 CP949 재인코딩** + 글리프 왕복 검증(`GetBytes`/`GetString` 대조)이 안전. Dino/RoadFighter가 이 방식으로 작성됨.
- **헤더는 되도록 ASCII(영문 주석)로** 두면 인코딩 이슈가 없다. 최근 신규 헤더들이 그 예.
- 프로젝트 파일 `CSGP.vcxproj` / `.vcxproj.filters`는 **UTF-8**이다(필터명 "소스 파일"/"헤더 파일"이 한글). 여긴 UTF-8 편집 OK.
- 사용 검증된 CP949 전각 글리프: `■`(U+25A0), `●`(U+25CF), `◆`(U+25C6), `★`(U+2605), `▲`(U+25B2), `♥`(U+2665). 수정 후 항상 `git diff --numstat`로 확인.

---

## 7. 새 씬(게임) 추가 절차

1. `Main/Content/`에 씬 클래스 생성 — `IContent`(단순) 또는 `IGameContent`(게임) 상속.
   - 전각/한글을 쓰면 **CP949로 저장**(신규는 UTF-8 작성 → CP949 재인코딩). 헤더는 ASCII 권장.
2. `framework.h`의 `enum class _ECONTENT`에 값 추가(끝에 추가해 기존 정수 순서 유지).
3. `MainContent::OnInit()`에서 `SCENE->AddContent((int)_ECONTENT::XXX, new XxxContent());` 등록.
4. `CSGP.vcxproj`(`<ClCompile>`/`<ClInclude>`)와 `CSGP.vcxproj.filters`에 새 파일 등록.
5. 시작 씬은 `MainContent::OnInit()` 끝의 `SCENE->ChangeContent(...)` 로 지정.

> **⚠️ Windows.h 이름 충돌 주의**: 프레임워크가 `Windows.h`를 포함하므로 흔한 식별자가 매크로/타입과 충돌할 수 있다. 이 세션에서 `ACCEL` 상수가 `winuser.h`의 `ACCEL` 타입과 충돌 → `ACCEL_RATE`로 개명해 해결. (다른 위험 후보: `RED/GREEN/BLUE`는 framework.h가 이미 색상수로 정의, `IN/OUT/NEAR/FAR/small` 등.)

> **현재 시작 씬 = `_ECONTENT::BATTLECITY`.** 게임을 만들 때마다 테스트 편의로 시작 씬을 그 게임으로 바꿔 왔다(…→ROADFIGHTER→MAZE→GALAGA→BATTLECITY 순으로 변경됨). 선택화면 완성 후 `MainContent.cpp`의 `ChangeContent` 한 줄을 원하는 씬(예: `TITLE`)으로 되돌리고, 선택화면에서 각 게임으로 `ChangeContent`로 진입시키면 된다.

---

## 8. ESC 뒤로가기 규칙 (게임 공통)

`main.cpp`가 ESC를 **`OnUpdate()` 뒤에서** 검사하도록 되어 있어, 씬이 먼저 ESC를 소비할 기회를 가진다. `OnKeyDown`이 엣지 트리거라 한 눌림당 한 곳만 `true`를 받는 특성을 이용:

- **인게임에서 ESC** → 씬이 ESC를 잡아 `currentPhase = TITLE`(타이틀로). main은 이미 소비된 ESC를 받아 종료하지 않음.
- **타이틀에서 ESC** → 씬이 ESC를 처리하지 않으므로 main이 잡아 프로그램 종료.
- 결과: "게임 → 타이틀 → 종료"의 자연스러운 단계적 뒤로가기. Tetris/Dino/RoadFighter가 이 규칙을 따른다.

---

## 9. 작업 내역

> **(A)~(C)는 커밋 완료**(`d50a4a8`: 프레임워크 픽스 · 신규 게임 4종 · GDD·문서). **(D) Galaga · Battle City · `Pool.h`는 아직 uncommitted** — 리뷰 후 커밋 필요. (브랜치 `main`)

### (A) 프레임워크/게임 버그 수정
- **ScreenManager**: 콘솔 창 rect의 `Top/Bottom` 뒤바뀜 수정, `ClearBuffer`에 색 속성 초기화 추가(색 잔상), `defaultBGColor` 니블 처리 수정.
- **TimerManager**: `SetFrame`의 busy-wait(CPU 100%)를 `Sleep` 기반으로 교체, ms를 `CLOCKS_PER_SEC`으로 나누던 단위 오류를 `/1000.0`으로 수정, `frameTime` 멤버 추가.
- **IContent**: 가상 소멸자 추가(delete 시 UB 방지).
- **SceneManager**: `OnRelease()`가 현재 씬 해제 + 등록 씬 전체 delete(누수 수정).
- **SnakeContent**: `life == 0` → `life <= 0`, 리스폰 지점 트랩/아이템 배치 제외, 자기 몸 충돌 판정 추가, 역주행 입력 차단.
- **LoadContent**: 두 공이 스쳐 지나갈 때 배열 범위 밖 쓰기 가드.
- `main.cpp`: 프레임 나눗셈 `1000/60`(정수) → `1000.0f/60.0f`, ESC 검사를 OnUpdate 뒤로 이동(§8).

### (B) 신규 게임 4종 (`Main/Content/`)
| 게임 | 파일 | 요약 |
|------|------|------|
| **Tetris** | `TetrisContent.*` | 표준 10×20, 7-bag, SRS 회전 테이블+월킥, 홀드/고스트/하드드롭, 라인클리어·점수·레벨, DAS 좌우 이동, 일시정지/게임오버 |
| **Dino** | `DinoContent.*` | 크롬 디노 러너. 점프(중력)/숙이기/빠른착지, 선인장·익룡 3높이, 속도 가속, 700점 밤낮, 100점 마일스톤(Beep), HI 스코어, 고스트/애니메이션 |
| **Road Fighter** | `RoadFighterContent.*` | 5차선 레이서. 차선 이동, 가속/감속, 적(일반/고속위빙/트럭)·아이템(연료◆/보너스★)·장애물▲, 연료·목숨·무적, 시간기반 스테이지 1~5, HUD |
| **Maze** | `MazeContent.*` | 절차적 미로 생성 5종 + 경로탐색 4종(BFS/DFS/A*/벽따라가기) 시각화, union-find·분할정복, 최단걸음 채점 |

- 3종 모두 `IGameContent` 상속, 값 타입 `std::vector`로 오브젝트 관리(누수 없음), ESC 뒤로가기 규칙(§8) 준수.
- 각 게임은 `framework.h`(`_ECONTENT`), `MainContent`(등록), `.vcxproj`/`.filters`에 등록됨.
- 설계 문서: `Docs/{TetrisContent,DinoContent,RoadFighterContent}_GDD.md`.

### (C) 문서
- `CLAUDE.md` — 작업 규칙(인코딩·아키텍처·새 씬 추가·빌드).
- `Docs/` — 게임 GDD + `CURRICULUM.md`(게임 7종 학습 지도).
- `handover.md` — 이 문서.

### (D) 이번 세션 추가 — Galaga · Battle City (uncommitted)
- **공용 `Pool.h`**: 제네릭 오브젝트 풀 `Pool<T,N>`(`Spawn`/`Kill`/`Clear`, `active` 플래그 재사용)을 별도 헤더로 분리. Galaga·Battle City가 공유(같은 TU에서 두 번 정의되는 충돌 방지). Galaga 헤더의 인라인 정의는 `#include "Pool.h"`로 리팩터.
- **Galaga** (`GalagaContent.*`, CP949): 편대 슈팅. 총알·적·폭발을 전부 풀로 관리(루프 내 `new/delete` 0회). 좌우행진+벽반사 하강 편대, 적 3종(색 구분), 강한 적 3방향 탄막, 무적 깜빡, 스테이지 진행(12/16/20…), 세션 HI. **편대 중심 MVP**(다이브 공격은 GDD §13로 보류).
- **Battle City** (`BattleCityContent.*`, CP949): 탱크 액션 **캡스톤**. 맵 절차생성(철벽 테두리+기지 벽돌요새+좌우대칭 벽돌/철벽/물/숲/얼음 + flood-fill 연결성 터널 carve) + 오브젝트 풀링(총알·적탱크·폭발·아이템). 4방향 회전(`▲▶▼◀`), 적 4종 AI(기지 편향 이동), 총알-타일/탱크/총알 충돌, 강화탄, 파워업 6종, 숲 시야가림, 기지 방어 승패, 스테이지 진행.
- **글리프 CP949 왕복 검증**: `▲▶▼◀ ● □ ■ ♣ ★ ♥ ◆ ◎ ▨` 통과. 물 `≈`(U+2248)·light shade는 실패 → **물 `▒`(U+2592)·얼음 `≡`(U+2261)** 전각으로 대체.
- **등록**: `framework.h`(`GALAGA`, `BATTLECITY`), `MainContent`(AddContent+시작씬), `.vcxproj`/`.filters`(3파일). 빌드 성공(경고 0, 기존 `main.cpp` C4244 제외), 타이틀 무크래시 스모크 통과.
- 이로써 **`Docs/CURRICULUM.md`의 게임 7종이 모두 코드 구현 완료.**

---

## 10. 알려진 이슈 · 다음 할 일

### 진행 중(사용자 담당)
- **게임 선택화면**: `TitleContent`에 구현 예정. 완성되면 시작 씬을 `TITLE`로 되돌리고, 메뉴에서 각 게임으로 `ChangeContent` 연결.

### 커밋
- **uncommitted = Galaga · Battle City · `Pool.h` + 등록(`framework.h`/`MainContent`/`.vcxproj`/`.filters`).** 주제별 분할 커밋 권장: ① `Pool.h` 분리 + Galaga, ② Battle City, ③ 등록·시작씬, ④ 문서(handover/CURRICULUM 갱신).

### 게임별 개선 후보
- **Tetris**: 락 딜레이, 라인클리어 연출, T-Spin, 최고점수 저장, 정식 SRS 킥.
- **Dino**: 락 아님(러너), 달 위상, 낮/밤 페이드, 최고점수 저장.
- **Road Fighter**: 자유 횡이동(현재 차선 스냅), 야간/우천, 경찰차, 최고기록 저장. 밸런스(SPEED·SCROLL_K·FUEL_DRAIN·스폰 간격) 튜닝은 실제 플레이 확인 필요.
- **Maze**: 생성/탐색 애니 속도 조절, 스테이지 확장, 최단경로 채점 튜닝.
- **Galaga**: 다이브 공격(GDD §13), 프리 리스트 O(1) 풀, 하이스코어 파일 저장. 밸런스(편대속도·발사간격) 실제 플레이 확인.
- **Battle City**: 물/숲 셀룰러 오토마타 스무딩, BFS 기반 적 AI(현재 랜덤+기지 편향), 2인 협동, 하이스코어 저장, 공간분할 충돌. 맵 생성 결과·밸런스는 실제 플레이 확인 필요.

### 프레임워크
- `SnakeContent`는 반복자를 클래스 멤버로 보관하는 설계 냄새(중첩 순회 위험) — 리팩터 후보.
- `IntroContent`는 스텁, `Intro`/`Load` 씬은 등록돼 있으나 현재 진입 경로 없음.
- 소스 전체를 UTF-8(+ `/utf-8`)로 통일하는 것은 별도 결정 사항(사용자 요청 시에만).

### 다음 게임/기술 후보 (커리큘럼 7종 완료 이후)
- **게임 선택 허브(`TitleContent`)** ⭐ 최우선 — 7게임을 한 화면에서 고르는 씬. 완성 시 시작 씬을 `TITLE`로 복귀(위 "진행 중" 항목과 동일).
- **저장/불러오기** — 하이스코어·설정 파일 I/O(직렬화 입문). 여러 게임에 공통 적용(`CURRICULUM.md` §12).
- **성능 심화** — Galaga/BC GDD가 예고한 프리 리스트 O(1) 풀 + 공간분할(그리드) 충돌 = "측정 후 최적화" 실습.
- **새 게임(8단계+)으로 새 기술**: Pac-Man(고스트 성격별 타게팅 AI·FSM) / Breakout(2D 벡터 반사 물리) / 타일 플랫포머(타일맵 충돌 해결·카메라) / Sokoban(파일 기반 레벨 로딩 + 실행취소 스택).

### 검증 한계
- 콘솔 게임 특성상 자동 검증은 빌드 성공 + 무크래시까지. **실제 플레이(조작감/렌더/밸런스)는 콘솔에서 직접 확인**해야 한다.

---

## 11. 참고 문서

- [`CLAUDE.md`](CLAUDE.md) — 작업 규칙(인코딩, 아키텍처, 새 씬 추가, 빌드).
- [`Docs/CURRICULUM.md`](Docs/CURRICULUM.md) — 게임 7종 학습 지도. 게임별 GDD: [`Snake`](Docs/SnakeContent_GDD.md)(as-built)·[`Tetris`](Docs/TetrisContent_GDD.md)·[`Dino`](Docs/DinoContent_GDD.md)·[`RoadFighter`](Docs/RoadFighterContent_GDD.md)·[`Maze`](Docs/MazeContent_GDD.md)·[`Galaga`](Docs/GalagaContent_GDD.md)·[`BattleCity`](Docs/BattleCityContent_GDD.md). 커리큘럼 이후 게임 백업: [`FUTURE_GAMES.md`](Docs/FUTURE_GAMES.md).
- [`README.md`](README.md) — 프레임워크 개요와 생명주기 예시.
