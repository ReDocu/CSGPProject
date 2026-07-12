# framework_update.md — CSGP 프레임워크 전면 개편안 (v0.2 → v1.0)

> **목표**: 기능(게임플레이·화면·조작)은 그대로 두고, 게임들에 흩어져 있는 **공통 코드를 프레임워크로 승격**해 "새 게임은 게임 고유 로직만 쓰면 되는" 상태를 만든다.
> **성격**: 행동 보존 리팩터링(behavior-preserving refactoring). 단계마다 빌드 + 회귀 체크리스트(§7)를 통과해야 다음 단계로 간다.
> 함께 볼 문서: [`CLAUDE.md`](CLAUDE.md)(인코딩 규칙) · [`handover.md`](handover.md) · [`Docs/CURRICULUM.md`](Docs/CURRICULUM.md)

작성일: 2026-07-12 · 기준 코드: 씬 12종(게임 9 + Intro/Title/Load), FrameWork 488줄 vs Main/Content **7,936줄**

---

## 1. 현재 상태 진단 — 중복 코드 카탈로그 (실측)

| # | 중복 항목 | 현재 위치 (실측) | 문제 |
|---|---|---|---|
| 1 | **타이틀 메뉴** (▶ 커서 + 게임시작/게임종료) | `s_titleSelect` 복붙 ×7 (Tetris·Dino·RF·Maze·Galaga·BC·PacMan) + enum 변형 ×2 (Snake·Tycoon) + 허브(Title) 변형 1 | 같은 로직 10벌. 메뉴 수정 시 10파일 수정 |
| 2 | **`Pad()`** (점수 0채움) | BattleCity·Galaga·PacMan·RoadFighter에 각각 정의 (×4) | 완전 동일 함수 4중 정의 |
| 3 | **`DrawSpriteRows()`** (여러 줄 스프라이트) | Dino·RoadFighter (×2) | 동일 함수 2중 정의 |
| 4 | **`StartGame()` 리셋 패턴** | 6개 게임 | 이름만 같고 베이스 훅 없음 → 타이틀 메뉴가 게임마다 직접 호출 |
| 5 | **일시정지 'P' 토글** | 7개 게임 | 동일한 3~6줄 복붙 |
| 6 | **인게임 ESC → 내부 타이틀** | 9개 게임 전부 | 동일한 5줄 복붙 (소비 규약 포함) |
| 7 | **`hiScore` 세션 보존** | 6개 게임 | 멤버 + 갱신 코드 반복 |
| 8 | **`x * 2` 전각 좌표 변환** | **9개 파일 전부** | 가장 실수 잦은 규약이 호출부에 노출 |
| 9 | **틱 타이머 float 키 충돌** | `GetTickTimer(0.1f)`를 **Snake와 Dino가 공유** | 키=float값 전역 공유. 씬 전환 직후 첫 틱이 이전 씬 타임스탬프의 영향을 받음 |
| 10 | **`Pool<T,N>`** | `Main/Content/Pool.h` | 엔진 성격인데 콘텐츠 계층에 위치 |
| 11 | **엔티티 구조체** (x,y,shape,color) | Snake의 Snake/Item/Trap/Tunnel 등 게임마다 유사 구조 반복 | 동일 필드 4개짜리 클래스가 사방에 |
| 12 | **전각 글리프 리터럴** ("■" "●" "▶"…) | 소스 곳곳에 CP949 리터럴 | 인코딩 사고의 진원지 (LoadContent 손상 사례 2회) |

> 결론: 이번에 타이틀 디자인 통일 작업이 10개 파일 수정이 된 것 자체가 증거다. **"한 번 고치면 한 곳"** 이 되도록 위 12개를 프레임워크로 올린다.

---

## 2. 개편 후 구조

```
FrameWork/
  Core/                          ← 기존 매니저 (동작 불변)
    singleton.h
    ScreenManager.h/.cpp             + DrawCell(논리좌표) 등 확장 (§3.3)
    TimerManager.h/.cpp              + TickTimer 핸들 (§3.6)
    InputManager.h/.cpp              (유지, 액션 매핑은 선택 §3.7)
    SceneManager.h/.cpp              (유지 — quit/reserve API는 이번에 이미 정식화됨)
  Interface/
    IContent.h                       (유지)
    IGameContent.h/.cpp          ← Main/Content/Interface 에서 승격 + 공통화 (§3.1)
  UI/
    SelectMenu.h/.cpp            ← ▶ 커서 메뉴 위젯 (§3.2)
    TextUtil.h/.cpp              ← Pad, Comma(콤마 표기), Center (§3.4)
  Render/
    RenderUtil.h/.cpp            ← DrawSpriteRows, DrawGauge, DrawBorder (§3.3)
    Glyph.h                      ← 전각 문자 상수 (§3.5)
  Object/
    Pool.h                       ← Main/Content 에서 이동 (내용 불변)
    Cell.h                       ← 공용 엔티티 구조체 (§3.5)

Main/
  MainContent.*                      (씬 등록 + 테두리 → DrawBorder 호출로 축소)
  Content/                           (게임 9종: 고유 로직만 남김)
```

- 기존 `framework.h`(싱글턴 매크로·색상·GAME_SIZE·_ECONTENT)는 **그대로 유지** — 전 파일이 의존하므로 건드리지 않는 게 안전. 새 헤더들만 추가로 include.
- `.vcxproj` 필터도 위 구조대로 정리(소스/헤더 2분류 → 폴더별 필터).

---

## 3. 모듈별 상세 설계

### 3.1 `IGameContent` 승격 + 공통 페이즈 로직 (중복 #1·4·5·6·7 해소)

현재 IGameContent는 페이즈 분기(TITLE/INGAME)만 갖고 있다. 여기에 **모든 게임이 복붙한 코드를 템플릿 메서드로** 올린다:

```cpp
// FrameWork/Interface/IGameContent.h  (발췌)
class IGameContent : public IContent {
protected:
    _EPhase currentPhase = _EPhase::NONE;
    SelectMenu titleMenu;              // "게임 시작 / 게임 종료"
    int  hiScore = 0;
    bool paused = false;

    // 프레임워크는 콘텐츠 enum을 모르므로 int 키로 주입받는다 (MainContent가 1회 설정)
    static int s_loadKey, s_hubKey;
public:
    static void SetSceneKeys(int loadKey, int hubKey);

    virtual void OnTitleUpdate();      // 공통: 메뉴 조작 → OnGameStart()/허브 복귀
    virtual void OnTitleRender();      // 공통: 메뉴 + 설명서 / 로고는 훅으로
    virtual void OnInGameUpdate();     // 공통: ESC→타이틀, P 일시정지 → OnGameUpdate()
protected:
    virtual void OnGameStart() = 0;            // 기존 StartGame() 들이 이 훅으로
    virtual void OnGameUpdate() = 0;           // 기존 OnInGameUpdate 본문
    virtual void OnTitleLogoRender() = 0;      // 로고 아트 (게임 고유)
    virtual const char** GetHelpLines(int& count) = 0;  // 설명서 텍스트 (게임 고유)
    virtual short GetMenuY() { return 14; }    // 로고 높이에 따른 메뉴 위치
    void UpdateHiScore(int score);             // hiScore 공통 갱신
};
```

- **게임이 제공하는 것은 데이터 3개**(로고 그리기, 설명서 문자열, 메뉴 y좌표)와 **훅 2개**(시작 리셋, 인게임 갱신)뿐.
- ESC 소비 규약("씬이 소비하면 main이 종료 안 함"), 게임 종료 → `ChangeContentWithLoading(s_loadKey, s_hubKey)` 로딩 경유가 베이스에 들어가므로 **규약 누락 실수가 원천 차단**된다.
- `MainContent::OnInit()`에서 `IGameContent::SetSceneKeys((int)_ECONTENT::LOAD, (int)_ECONTENT::TITLE);` 1회 호출 — 프레임워크는 계속 `_ECONTENT`를 모른다(계층 규칙 유지).
- 일시정지를 안 쓰는 게임(Snake·Tycoon)은 `virtual bool UsePause() { return true; }` 를 false로 오버라이드.

### 3.2 `SelectMenu` — ▶ 커서 메뉴 위젯 (중복 #1)

타이틀 2항목, 허브 2열 10항목, Tycoon 인게임 6항목이 전부 같은 패턴이므로 위젯 하나로:

```cpp
class SelectMenu {
public:
    void SetItems(const char** items, int count, int columns = 1);
    int  Update();                       // 방향키 이동, Enter시 선택 index 반환 (아니면 -1)
    void Render(short x, short y, short stepY, unsigned short itemColor = BLUE);
    int  Cursor() const;  void Reset();
};
```

- 렌더 규약은 이번에 통일한 디자인 그대로: 항목 BLUE, 커서 `▶` RED(항목 x-4 위치).
- 허브(TitleContent)는 `columns=2`로, Tycoon 인게임 메뉴도 이 위젯으로 교체 가능(선택).

### 3.3 `ScreenManager` 확장 + `RenderUtil` (중복 #3·8)

```cpp
// ScreenManager 에 추가 (기존 OnDraw/OnDrawColor 는 그대로 둔다)
void DrawCell(short lx, short y, const char* glyph, unsigned short color, unsigned short bg = 255);
//  = OnDrawColor(lx * 2, y, ...)  ← "출력 시 x*2" 규약을 엔진 안으로

// FrameWork/Render/RenderUtil.h
namespace RenderUtil {
    void DrawSpriteRows(short lx, short y, const char* const* rows, int n, unsigned short color);
    void DrawGauge(short x, short y, int val, int maxv, int width);   // [####----]
    void DrawBorder(unsigned short color = GRAY);                     // MainContent의 테두리 이동
}
```

- 게임 로직은 계속 논리 좌표(40×25)로 계산하고, **`* 2`는 호출부에서 사라진다**. 기존 `OnDraw(x*2, …)` 호출은 단계적으로 `DrawCell(x, …)`로 교체 (동작 동일).
- ASCII 텍스트(HUD 등)는 기존처럼 콘솔 원좌표 `OnDraw` 사용 — 두 API의 용도 구분을 헤더 주석으로 명시.

### 3.4 `TextUtil` (중복 #2)

```cpp
namespace TextUtil {
    std::string Pad(int v, int width);        // 4중 정의 통합
    std::string Comma(int v);                 // Tycoon MoneyText 승격 (1,234)
    short Center(int textBytes);              // 80칸 기준 중앙 x = 40 - bytes/2
}
```

### 3.5 `Glyph.h` + `Cell.h` (중복 #11·12, 인코딩 리스크 축소)

```cpp
// FrameWork/Render/Glyph.h  — 파일 자체는 100% ASCII (CP949 바이트를 이스케이프로)
#define GLYPH_BLOCK   "\xA1\xE1"   /* filled square  (U+25A0) */
#define GLYPH_BALL    "\xA1\xDd"   /* filled circle  (U+25CF) — 실제 바이트는 이관 시 확정 */
#define GLYPH_CURSOR  "\xA1\xE4"   /* right triangle (U+25B6) */
#define GLYPH_STAR    "\xA1\xD9"   /* star           (U+2605) */
// ... 사용 중인 전각 글리프 전수(▼▲□◆♠Ω● 등)를 이관하며 확정
```

- **왜 이스케이프인가**: LoadContent가 두 번 깨진 원인이 전각 리터럴 + 에디터 인코딩이었다. 프레임워크 파일을 ASCII로 유지하면 이 사고가 구조적으로 불가능해진다. 실제 바이트 값은 이관 작업 때 기존 CP949 파일에서 추출해 확정한다.
- 게임 소스의 리터럴은 단계적으로 상수로 교체(강제는 아님 — 한글 문자열은 어차피 CP949 유지).

```cpp
// FrameWork/Object/Cell.h — Snake의 Snake/Item/Trap/Tunnel 류 통합용 (opt-in)
struct Cell {
    int x = 0, y = 0;
    std::string shape;
    unsigned short color = WHITE;
};
```

### 3.6 `TimerManager` — 틱 타이머 키 충돌 해소 (중복 #9)

문제: 키가 float 값 그 자체라 **Snake와 Dino가 0.1f 타이머를 공유** 중. 해법은 씬이 타이머를 소유하는 핸들 방식:

```cpp
class TickTimer {
    float interval; unsigned long long last = 0;
public:
    explicit TickTimer(float sec) : interval(sec) {}
    bool Tick();      // GetTickCount64 기반, 상태는 인스턴스에
    void Reset();
};
// 사용: 멤버로 TickTimer moveTimer{0.1f};  → OnInit에서 Reset(), 루프에서 if (moveTimer.Tick())
```

- 씬 재진입 시 OnInit에서 Reset → "이전 씬의 타임스탬프" 문제 자동 해결.
- 기존 `TIMER->GetTickTimer(float)`는 **당분간 유지**(deprecated 주석) — 게임들을 하나씩 옮기고 전부 끝나면 제거. `SetFrame`/`GetContentTime`은 그대로.

### 3.7 `InputManager` — 액션 매핑 (선택, 마지막 단계)

WASD(BC·PacMan) vs 방향키(Snake·Tetris·Maze) vs A/D(Galaga)로 조작이 갈라져 있다. `IsAction(ACT_LEFT)` 식 매핑 테이블을 도입하면 통일 가능하지만, **키 바인딩 변화 = 기능 변화**이므로 이번 개편 범위에서는 **설계만 잡고 적용은 보류**(기능 불변 원칙). 도입하더라도 게임별 opt-in.

### 3.8 `SceneManager` — 현행 유지

이번 세션에서 추가된 것들이 이미 개편 방향과 일치: `RequestQuit/IsQuitRequested`(메뉴 종료), `Reserve/Peek/PopReservedContent` + `ChangeContentWithLoading`(로딩 경유 전환). 주석 보강과 Core/ 이동만 한다.

---

## 4. Before / After — 새 게임의 타이틀 코드

```cpp
// [Before] 게임마다 ~60줄 (메뉴 로직 + 렌더 + ESC/P/hiScore 각자 구현)
static int s_titleSelect = 0;
void FooContent::OnTitleUpdate() { /* 커서 이동, Enter 분기, 허브 복귀 ... 20줄 */ }
void FooContent::OnTitleRender() { /* 로고 + 커서 + 메뉴 + 설명서 ... 25줄 */ }
void FooContent::OnInGameUpdate() { /* ESC 소비, P 토글, 본문 ... */ }

// [After] 게임은 고유 데이터/로직만 (~15줄 + 본문)
void FooContent::OnGameStart()        { /* 리셋만 */ }
void FooContent::OnGameUpdate()       { /* 게임 본문만 */ }
void FooContent::OnTitleLogoRender()  { /* 로고 아트만 */ }
const char** FooContent::GetHelpLines(int& n) { static const char* h[] = { "...조작...", "...목표..." }; n = 2; return h; }
```

새 씬 추가 절차(CLAUDE.md §새 씬 추가)도 "IGameContent 상속 + 훅 4개 구현 + enum/등록/vcxproj"로 짧아진다.

---

## 5. 마이그레이션 플랜 — 6단계 (기능 동일 보장)

| Phase | 내용 | 변경 범위 | 위험 | 완료 기준 |
|:---:|---|---|:---:|---|
| **0** | **기준선 확보**: 현재 미커밋 변경(타이틀 통일·로딩 시퀀스·Tycoon 등) 커밋, §7 체크리스트 1회 전체 수행·기록 | git만 | 없음 | 체크리스트 결과 기록 |
| **1** | **무해한 추출**: TextUtil(Pad·Comma), RenderUtil(DrawSpriteRows·DrawGauge·DrawBorder), Glyph.h, Pool.h 이동, DrawCell 추가 | 신규 파일 + 호출부 치환 (게임 로직 무변경) | 하 | 빌드 + 체크리스트 |
| **2** | **타이틀 공통화**: SelectMenu 도입, IGameContent 승격(SetSceneKeys), 게임 9종 타이틀 Update/Render → 훅/데이터로 전환. 허브(TitleContent)도 SelectMenu로 | IGameContent + 게임 9종 + Title | **중** | 전 게임 타이틀 조작 확인 |
| **3** | **인게임 공통화**: ESC 소비·P 일시정지·hiScore를 베이스로. 각 게임은 OnGameUpdate만 | IGameContent + 게임 7~9종 | 중 | ESC/P/점수 회귀 확인 |
| **4** | **TickTimer 전환**: 씬별 타이머 멤버화, 0.1f 충돌 해소. 구 API deprecated | TimerManager + 게임별 소량 | 하 | 이동/애니 속도 눈 확인 |
| **5** | **정리(선택)**: 폴더 재배치(Core/UI/Render/Object), vcxproj 필터, InputManager 액션 매핑 설계 문서화, ver 0.2→1.0 표기, handover.md 갱신 | 프로젝트 파일 | 하 | 클린 빌드 |

- **각 Phase = 커밋 1개 이상**, 커밋 전 반드시: 빌드 성공 + §7 체크리스트 해당 항목 통과.
- Phase 2·3이 핵심이자 최대 위험 구간 → 게임 1종(Snake)으로 먼저 파일럿 전환 후 나머지 8종에 확산하는 것을 권장.

---

## 6. 인코딩 정책 (이번 개편의 안전 수칙)

1. **FrameWork/ 이하 신규 파일은 100% ASCII** — 한글 주석 금지, 전각 리터럴은 Glyph.h 이스케이프로. (LoadContent 2회 손상의 재발 방지 — 프레임워크가 ASCII면 어떤 에디터로 열어도 안전)
2. 게임(.cpp)의 한글 문자열은 지금처럼 CP949 유지. 수정은 CLAUDE.md의 PowerShell 절차 준수.
3. 이관 작업 중 파일 단위로 `iconv -f cp949 -t utf-8 <file> > /dev/null` (왕복 검증) + `git diff --numstat` 확인을 습관화.

---

## 7. 회귀 체크리스트 (각 Phase 후 수행)

**공통 (허브/전환)**
- [ ] 허브: 방향키/←→ 커서 이동, Enter로 각 게임 진입, 숫자키 1~9 직행, EXIT로 정상 종료, ESC로 종료
- [ ] 씬 전환 시 로딩 화면 경유(NOW LOADING + 게임 이름), 로딩 중 ESC = 스킵(앱 종료 아님)
- [ ] 게임 내부 타이틀: ▶ 커서 메뉴, 게임 종료 → 로딩 → 허브

**게임별 최소 확인 (각 1~2분)**
- [ ] Snake: 이동/아이템★/트랩♠/터널□/생명, SPACE 타이틀 복귀
- [ ] Tetris: 회전(월킥)/하드드롭/홀드/라인 클리어/DAS, P 일시정지
- [ ] Dino: 점프/숙이기/장애물 충돌/가속, 밤낮 전환
- [ ] RoadFighter: 차선 이동/연료◆/충돌 무적/스테이지 전환
- [ ] Maze: 5스테이지 생성/H 힌트/F 자동풀이/TAB 알고리즘/별점
- [ ] Galaga: 이동/발사/편대 전멸→다음 웨이브/hiScore 유지
- [ ] BattleCity: 이동/발사/벽돌 파괴/기지 방어/게임오버
- [ ] PacMan: 펠릿/파워펠릿 반격/고스트 4성격/스테이지 클리어
- [ ] Tycoon: 기획→개발→출시→판매 1사이클, 고용/해고, 이벤트, 파산/승리 엔딩

**공통 (인게임 규약)**
- [ ] 전 게임: 인게임 ESC → 내부 타이틀 (앱 종료 아님), 타이틀 ESC → 앱 종료
- [ ] hiScore가 세션 내 유지되는 6종 확인

---

## 8. 하지 않을 것 (Out of Scope)

- 게임플레이/밸런스/조작 변경 (액션 매핑 포함 — 설계만)
- 해상도(40×25)·색상 팔레트 변경, UTF-8 전환, 외부 라이브러리 도입
- 씬 소유권 모델 변경 (SceneManager가 소유·해제하는 현행 유지)
- Study/(1부 C 학습 코드) — 이번 개편과 무관, 손대지 않음

---

## 9. 기대 효과 요약

| 지표 | 현재 | 개편 후 (추정) |
|---|---|---|
| 타이틀 메뉴 수정 시 손대는 파일 | 10개 | **1개** (SelectMenu) |
| 새 게임 타이틀/공통 코드 | ~60줄 복붙 | **~15줄** (훅 + 데이터) |
| 동일 유틸 중복 정의 | Pad ×4, Sprite ×2 등 | **0** |
| `x*2` 규약 노출 | 9개 파일 | 엔진 내부로 은닉 |
| 타이머 키 충돌 | Snake↔Dino 0.1f 공유 | 씬 소유 TickTimer로 해소 |
| 인코딩 사고 표면적 | 전각 리터럴 산재 | 프레임워크 ASCII화 + Glyph.h |

> 승인되면 Phase 0(현재 변경 커밋 + 기준선)부터 시작한다. Phase 2·3은 Snake 파일럿 → 8종 확산 순서를 권장.
