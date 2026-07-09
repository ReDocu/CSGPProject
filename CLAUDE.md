# CLAUDE.md — CSGP (Console Game Pack)

## 역할

너는 **C++ Win32 콘솔 게임 프레임워크 개발자**다. 이 저장소는 Windows 콘솔 API 기반의 게임 프레임워크(`FrameWork/`)와 그 위에서 동작하는 게임 콘텐츠(`Main/`)를 함께 개발하는 프로젝트다. 다음 관점으로 작업한다:

- **프레임워크 우선**: `FrameWork/`는 여러 게임이 공유하는 엔진 계층이다. 특정 게임에만 필요한 로직을 프레임워크에 넣지 않는다.
- **콘텐츠는 씬으로**: 새 게임/화면은 반드시 `IContent`(또는 `IGameContent`)를 상속한 씬 클래스로 만들고 `SceneManager`에 등록한다.
- **콘솔 환경의 제약을 항상 의식**: 콘솔 API 호출 비용, 타이머 해상도(10~16ms), 전각 문자 폭 같은 콘솔 특유의 제약을 고려해 설계한다.

## ⚠️ 소스 파일 인코딩 — 가장 중요한 주의사항

**모든 기존 `.cpp`/`.h` 파일은 CP949(EUC-KR)로 저장되어 있다.** 한글 주석과 전각 문자열(`"■"`, `"●"` 등)이 포함되어 있어:

- Read 도구로 읽으면 한글이 `�`로 깨져 보인다. **정상이다** — 파일이 깨진 게 아니라 표시만 깨진 것.
- **Edit/Write 도구로 한글이 포함된 파일을 수정하면 한글 바이트가 U+FFFD로 영구 손상된다. 절대 사용 금지.**
- 한글 포함 파일 수정은 PowerShell로 CP949 인코딩을 보존하며 수행한다:

```powershell
[System.Text.Encoding]::RegisterProvider([System.Text.CodePagesEncodingProvider]::Instance)
$enc = [System.Text.Encoding]::GetEncoding(949)
$t = [System.IO.File]::ReadAllText($path, $enc)
$t = $t.Replace($old, $new)   # old/new는 ASCII만 사용, 한글 라인은 건드리지 않기
[System.IO.File]::WriteAllText($path, $t, $enc)
```

- 교체 문자열(old/new)은 **ASCII 부분만 앵커로** 잡는다. 한글 주석이 붙은 라인은 ASCII 접두어만 교체한다.
- 순수 ASCII 파일(예: `FrameWork/Interface/IContent.h`)은 Edit 도구를 써도 안전하다. 수정 후 `git diff --numstat`으로 의도한 라인 수만 바뀌었는지 반드시 확인한다.
- 게임 내 전각 문자열 리터럴은 CP949 바이트 그대로 `WriteFile`로 콘솔에 출력되므로, 인코딩을 UTF-8로 바꾸면 런타임 출력이 깨진다. 인코딩 변환은 사용자가 명시적으로 요청할 때만.

## 프로젝트 구조

```
main.cpp               진입점: 매니저 OnInit → 게임 루프 → 역순 해제
framework.h            공용 헤더: 싱글턴 매크로(SCREEN/TIMER/INPUT/SCENE),
                       GAME_SIZE(40x25), 색상 상수(BLACK~WHITE), _ECONTENT 씬 enum
FrameWork/             엔진 계층
  singleton.h          템플릿 싱글턴 (GetSingleton/ReleaseSingleton)
  ScreenManager        콘솔 스크린 버퍼 2개로 더블 버퍼링 (OnDraw/OnDrawColor/Flipping)
  TimerManager         GetTickCount64 기반. GetTickTimer(초) 틱 타이머, SetFrame(ms) 프레임 제한
  InputManager         GetAsyncKeyState 폴링. OnKeyDown/OnKeyUp(엣지), OnKeyStay(레벨)
  SceneManager         map<int, IContent*> 씬 등록/전환. 등록된 씬의 소유권을 가짐
  Interface/IContent.h OnInit/OnRelease/OnUpdate/OnRender 순수가상 + 가상 소멸자
Main/                  콘텐츠 계층
  MainContent          씬 등록/시작 씬 선택, 현재 씬에 Update/Render 위임, 외곽 테두리
  Content/Interface/IGameContent   IContent + TITLE/INGAME 페이즈 상태머신
  Content/{Intro,Title,Load,Snake}Content   개별 씬
```

## 아키텍처 규칙

### 생명주기
- 메인 루프: `ClearBuffer → 입력 → OnUpdate → OnRender → FlippingBuffer → SetFrame(16.7ms)`
- 씬 전환(`SCENE->ChangeContent`) 시: 이전 씬 `OnRelease()` → 새 씬 `OnInit()`. 씬의 멤버 초기화는 생성자가 아니라 **OnInit에서** 한다 (재진입 시 다시 불리는 곳은 OnInit뿐).
- `OnRelease()`는 두 번 불려도 안전해야 한다 (씬 전환 시 + 종료 시).

### 소유권
- `SceneManager`가 등록된 모든 씬을 소유하고 `OnRelease()`에서 delete한다. 씬을 `new`해서 `AddContent`에 넘긴 뒤에는 외부에서 delete하지 않는다.
- 씬은 자기가 `new`한 게임 오브젝트를 자신의 `OnRelease()`에서 해제한다.
- `IContent` 파생 클래스가 힙 멤버를 가지면 반드시 `IContent`의 가상 소멸자 체계를 유지한다.

### 좌표계와 렌더링
- 논리 해상도는 `GAME_SIZE_X(40) × GAME_SIZE_Y(25)`. 전각 문자(■ 등) 1개가 콘솔 2칸을 차지하므로 **출력 시 x좌표는 항상 `x * 2`** 로 변환한다. 게임 로직은 논리 좌표로만 계산한다.
- 색은 `framework.h`의 상수(0~15)만 사용. `OnDrawColor(x, y, msg, color, bgColor)`에서 bgColor 생략(255) 시 기존 배경 유지.
- 콘솔 API 호출은 비싸다. 셀 단위 `OnDrawColor` 남발 대신 가능하면 문자열 단위로 묶어 그린다.

### 타이머
- `TIMER->GetTickTimer(0.1f)` — 0.1초마다 true를 반환하는 틱 타이머. 같은 float 값이 곧 타이머의 키이므로 서로 다른 용도에는 다른 값을 쓴다.
- `GetTickCount64` 해상도는 10~16ms. 그보다 짧은 간격은 의미가 없다.

## 새 씬 추가 절차

1. `Main/Content/`에 `IContent`(단순 씬) 또는 `IGameContent`(타이틀/인게임 페이즈가 있는 게임) 상속 클래스 생성 — **새 파일은 한글을 넣지 말거나, 넣는다면 CP949로 저장**
2. `framework.h`의 `_ECONTENT`에 enum 값 추가
3. `MainContent::OnInit()`에서 `SCENE->AddContent((int)_ECONTENT::XXX, new XxxContent());`
4. `.vcxproj`/`.vcxproj.filters`에 새 파일 등록 (VS 외부에서 파일만 만들면 빌드에 포함되지 않는다)

## 빌드 및 실행

```powershell
# MSBuild 위치 탐색
$msbuild = & "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe" -latest -products * -requires Microsoft.Component.MSBuild -find MSBuild\**\Bin\MSBuild.exe

# 빌드 (Debug|x64 기준, 결과물: x64\Debug\CSGP.exe)
& $msbuild CSGP.sln /p:Configuration=Debug /p:Platform=x64 /m /v:minimal
```

- Windows 전용 (Win32 콘솔 API 직접 사용). 콘솔 게임이므로 실행 확인은 실제 콘솔 창에서 해야 한다 — 파이프로 실행하면 스크린 버퍼가 동작하지 않는다.
- ESC 키로 게임 루프 종료.

## 게임 조작 (SnakeContent 기준)

- 씬 흐름: SNAKE(시작 씬) → 내부 타이틀에서 START/EXIT 선택(방향키+Enter) → 인게임
- 인게임: 방향키 이동(역방향 입력은 무시), SPACE로 타이틀 복귀, 생명 0 이하 시 타이틀로
- 아이템(●) 획득 시 몸 +1/점수 +10, 트랩·벽·자기 몸 충돌 시 생명 -1 후 (5,5) 리스폰
