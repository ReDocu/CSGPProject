#include "TycoonContent.h"
#include "../../framework.h"
#include <cstdio>
#include <cstring>

// ===== 정적 데이터 =====
static const int GENRE_N = 5;
static const char* GENRE_NAME[GENRE_N] = { "RPG", "액션", "퍼즐", "시뮬", "스포츠" };
static const char* SCALE_NAME[3] = { "소형", "중형", "대형" };
static const int SCALE_COST[3] = { 200,  500, 1200 };	// 기획비
static const int SCALE_NEED[3] = { 100,  200,  400 };	// 필요 진행도
static const int SCALE_SALE[3] = { 1,    2,    4 };	// 판매 배수
static const int SCALE_PROFIT[3] = { 1,    2,    3 };	// 장당 이익 배수

static const char* COMPANY_NAME = "EQMENT STUDIO";

static const char* STAFF_NAME[16] = {
	"김개발", "이코딩", "박버그", "최야근", "정디버그", "한알고",
	"오포인터", "유메모리", "신렌더", "장서버", "임클라", "송기획",
	"백그래픽", "문사운드", "양테스트", "황빌드"
};

static const char* TITLE_HEAD[10] = {
	"용사의", "전설의", "우주", "좀비", "마법",
	"고양이", "던전", "레트로", "무한", "김치"
};
static const char* TITLE_TAIL[10] = {
	"모험", "라면", "파이터", "타이쿤", "레이싱",
	"퍼즐", "야구", "온라인", "서바이벌", "키우기"
};

// ===== 생명주기 =====
void TycoonContent::OnInit()
{
	IGameContent::OnInit();

	currentPhase = _EPhase::TITLE;
	select = _ESELECT::START;

	NewGame();
}

void TycoonContent::OnRelease()
{
	IGameContent::OnRelease();

	staffList.clear();
	applicants.clear();
	shelf.clear();
	reportMsgs.clear();
	reportMoney.clear();
}

void TycoonContent::NewGame()
{
	view = _EVIEW::MAIN;
	money = START_MONEY;
	fame = 0;
	year = START_YEAR;
	month = 1;

	staffList.clear();
	applicants.clear();
	shelf.clear();

	proj = Project();
	proj.active = false;

	ending = _EENDING::NONE;
	achievedRich = achievedMaster = false;
	bestRating = 0.0f;

	mainCursor = devCursor = staffCursor = 0;
	pickGenre = pickScale = 0;
	earlyPending = false;
	firePending = -1;
	shelfScroll = eventCursor = endCursor = 0;
	raiseIdx = -1;
	uiMsg.clear();
	reportMsgs.clear();
	reportMoney.clear();

	UpdateTrend(false);
	RollApplicants();
}

// ===== 타이틀 페이즈 =====
void TycoonContent::OnTitleUpdate()
{
	if (INPUT->OnKeyDown(VK_DOWN))
	{
		select = (_ESELECT)(((int)select) + 1);
		if (select == _ESELECT::END)
			select = _ESELECT::START;
	}

	if (INPUT->OnKeyDown(VK_UP))
	{
		select = (_ESELECT)(((int)select) - 1);
		if (select == _ESELECT::NONE)
			select = _ESELECT::EXIT;
	}

	if (INPUT->OnKeyDown(VK_RETURN))
	{
		switch (select)
		{
		case _ESELECT::START:
			NewGame();
			currentPhase = _EPhase::INGAME;
			break;
		case _ESELECT::EXIT:
			SCENE->ChangeContentWithLoading((int)_ECONTENT::LOAD, (int)_ECONTENT::TITLE);
			break;
		default:
			break;
		}
	}
}

void TycoonContent::OnTitleRender()
{
	SCREEN->OnDrawColor(22, 4, "■■■■■■■■■■■■■■■■■", DARKYELLOW);
	SCREEN->OnDrawColor(25, 6, "G A M E   D E V   T Y C O O N", YELLOW);
	SCREEN->OnDraw(28, 8, "- 게임회사 운영하기 -");
	SCREEN->OnDrawColor(22, 10, "■■■■■■■■■■■■■■■■■", DARKYELLOW);

	SCREEN->OnDrawColor(25, select == _ESELECT::EXIT ? 17 : 14, "▶", RED);
	SCREEN->OnDrawColor(32, 14, "게 임 시 작", BLUE);
	SCREEN->OnDrawColor(32, 17, "게 임 종 료", BLUE);

	SCREEN->OnDrawColor(16, 21, "자산 1억 달성 or 평점 9.0 명작 출시 = 승리 / 파산 = 패배", GRAY);
	SCREEN->OnDrawColor(20, 22, "방향키: 이동   Enter: 선택   ESC: 뒤로", GRAY);
}

// ===== 인게임: 입력 =====
void TycoonContent::OnInGameUpdate()
{
	// ESC = 뒤로 (씬이 소비하면 프로그램이 종료되지 않는다)
	if (INPUT->OnKeyDown(VK_ESCAPE))
	{
		switch (view)
		{
		case _EVIEW::MAIN:
			currentPhase = _EPhase::TITLE;
			select = _ESELECT::START;
			return;
		case _EVIEW::DEV:
		case _EVIEW::STAFF:
		case _EVIEW::STATUS:
		case _EVIEW::SHELF:
			uiMsg.clear();
			view = _EVIEW::MAIN;
			return;
		case _EVIEW::REPORT:
			endCursor = 0;
			view = (ending != _EENDING::NONE) ? _EVIEW::ENDING : _EVIEW::MAIN;
			return;
		default:	// EVENT / ENDING 은 Enter로 선택해야 한다
			return;
		}
	}

	switch (view)
	{
	case _EVIEW::MAIN:   UpdateMain();   break;
	case _EVIEW::DEV:    UpdateDev();    break;
	case _EVIEW::STAFF:  UpdateStaff();  break;
	case _EVIEW::STATUS: UpdateStatus(); break;
	case _EVIEW::SHELF:  UpdateShelf();  break;
	case _EVIEW::EVENT:  UpdateEvent();  break;
	case _EVIEW::REPORT: UpdateReport(); break;
	case _EVIEW::ENDING: UpdateEnding(); break;
	}
}

void TycoonContent::UpdateMain()
{
	if (INPUT->OnKeyDown(VK_UP))
		mainCursor = (mainCursor + 5) % 6;
	if (INPUT->OnKeyDown(VK_DOWN))
		mainCursor = (mainCursor + 1) % 6;

	if (INPUT->OnKeyDown(VK_RETURN))
	{
		uiMsg.clear();
		switch (mainCursor)
		{
		case 0: view = _EVIEW::DEV;   devCursor = 0; earlyPending = false; break;
		case 1: view = _EVIEW::STAFF; staffCursor = 0; firePending = -1;   break;
		case 2: view = _EVIEW::STATUS; break;
		case 3: view = _EVIEW::SHELF; shelfScroll = 0; break;
		case 4: StartMonth(); break;
		case 5:
			currentPhase = _EPhase::TITLE;
			select = _ESELECT::START;
			break;
		}
	}
}

void TycoonContent::UpdateDev()
{
	if (!proj.active)
	{
		if (INPUT->OnKeyDown(VK_UP))   devCursor = (devCursor + 3) % 4;
		if (INPUT->OnKeyDown(VK_DOWN)) devCursor = (devCursor + 1) % 4;

		if (devCursor == 0)
		{
			if (INPUT->OnKeyDown(VK_LEFT))  pickGenre = (pickGenre + GENRE_N - 1) % GENRE_N;
			if (INPUT->OnKeyDown(VK_RIGHT)) pickGenre = (pickGenre + 1) % GENRE_N;
		}
		if (devCursor == 1)
		{
			if (INPUT->OnKeyDown(VK_LEFT))  pickScale = (pickScale + 2) % 3;
			if (INPUT->OnKeyDown(VK_RIGHT)) pickScale = (pickScale + 1) % 3;
		}

		if (INPUT->OnKeyDown(VK_RETURN))
		{
			if (devCursor == 2)		// 개발 시작
			{
				int cost = SCALE_COST[pickScale];
				if (money < cost)
				{
					uiMsg = "! 자본금이 부족합니다. (기획비 " + MoneyText(cost) + "만원)";
				}
				else
				{
					money -= cost;
					proj = Project();
					proj.title = MakeTitle();
					proj.genre = pickGenre;
					proj.scale = pickScale;
					proj.need = SCALE_NEED[pickScale];
					proj.active = true;
					uiMsg = "* \"" + proj.title + "\" 개발 시작! (다음 달부터 진행)";
				}
			}
			else if (devCursor == 3)
			{
				uiMsg.clear();
				view = _EVIEW::MAIN;
			}
		}
	}
	else
	{
		if (INPUT->OnKeyDown(VK_UP) || INPUT->OnKeyDown(VK_DOWN))
		{
			devCursor = (devCursor == 0) ? 1 : 0;
			earlyPending = false;
		}

		if (INPUT->OnKeyDown(VK_RETURN))
		{
			if (devCursor == 0)		// 조기 출시
			{
				if (!earlyPending)
				{
					earlyPending = true;
				}
				else
				{
					earlyPending = false;
					reportMsgs.clear();
					reportMoney.clear();
					reportTitle = "출시 결과";
					DoRelease(true);
					CheckEnding();
					reportBefore = money;
					reportNet = 0;
					view = _EVIEW::REPORT;
				}
			}
			else
			{
				uiMsg.clear();
				view = _EVIEW::MAIN;
			}
		}
	}
}

void TycoonContent::UpdateStaff()
{
	int staffN = (int)staffList.size();
	int appN = (int)applicants.size();
	int total = staffN + appN + 1;		// + [뒤로]

	if (INPUT->OnKeyDown(VK_UP))
	{
		staffCursor = (staffCursor + total - 1) % total;
		firePending = -1;
	}
	if (INPUT->OnKeyDown(VK_DOWN))
	{
		staffCursor = (staffCursor + 1) % total;
		firePending = -1;
	}

	if (INPUT->OnKeyDown(VK_RETURN))
	{
		if (staffCursor < staffN)		// 직원 -> 해고 (두 번 확인)
		{
			if (firePending == staffCursor)
			{
				money -= staffList[staffCursor].salary;		// 퇴직금
				uiMsg = "* [" + staffList[staffCursor].name + "] 해고 (퇴직금 지급)";
				staffList.erase(staffList.begin() + staffCursor);
				firePending = -1;
				if (staffCursor > 0) staffCursor--;
			}
			else
			{
				firePending = staffCursor;
				uiMsg.clear();
			}
		}
		else if (staffCursor < staffN + appN)	// 지원자 -> 고용
		{
			int ai = staffCursor - staffN;
			if ((int)staffList.size() >= MAX_STAFF)
			{
				uiMsg = "! 사무실이 가득 찼습니다. (최대 8명)";
			}
			else
			{
				staffList.push_back(applicants[ai]);
				uiMsg = "* [" + applicants[ai].name + "] 고용!";
				applicants.erase(applicants.begin() + ai);
			}
			firePending = -1;
		}
		else
		{
			uiMsg.clear();
			view = _EVIEW::MAIN;
		}
	}
}

void TycoonContent::UpdateStatus()
{
	if (INPUT->OnKeyDown(VK_RETURN))
		view = _EVIEW::MAIN;
}

void TycoonContent::UpdateShelf()
{
	const int visible = 13;
	int maxScroll = (int)shelf.size() - visible;
	if (maxScroll < 0) maxScroll = 0;

	if (INPUT->OnKeyDown(VK_UP) && shelfScroll > 0)
		shelfScroll--;
	if (INPUT->OnKeyDown(VK_DOWN) && shelfScroll < maxScroll)
		shelfScroll++;

	if (INPUT->OnKeyDown(VK_RETURN))
		view = _EVIEW::MAIN;
}

void TycoonContent::UpdateEvent()
{
	if (INPUT->OnKeyDown(VK_LEFT) || INPUT->OnKeyDown(VK_RIGHT) ||
		INPUT->OnKeyDown(VK_UP) || INPUT->OnKeyDown(VK_DOWN))
		eventCursor = 1 - eventCursor;

	if (INPUT->OnKeyDown(VK_RETURN))
	{
		if (raiseIdx >= 0 && raiseIdx < (int)staffList.size())
		{
			Staff& s = staffList[raiseIdx];
			if (eventCursor == 0)	// 수락
			{
				s.salary = s.salary * 12 / 10;
				s.months = 0;
				reportMsgs.push_back("[이벤트] " + s.name + " 월급 인상 수락 (" + MoneyText(s.salary) + "만원)");
			}
			else					// 거절 -> 퇴사
			{
				reportMsgs.push_back("[이벤트] " + s.name + " 이(가) 회사를 떠났습니다...");
				staffList.erase(staffList.begin() + raiseIdx);
			}
		}
		raiseIdx = -1;
		FinishMonth();
	}
}

void TycoonContent::UpdateReport()
{
	if (INPUT->OnKeyDown(VK_RETURN))
	{
		endCursor = 0;
		view = (ending != _EENDING::NONE) ? _EVIEW::ENDING : _EVIEW::MAIN;
	}
}

void TycoonContent::UpdateEnding()
{
	if (ending == _EENDING::BANKRUPT)
	{
		if (INPUT->OnKeyDown(VK_RETURN))
		{
			currentPhase = _EPhase::TITLE;
			select = _ESELECT::START;
		}
		return;
	}

	// 승리 엔딩: 계속하기 선택 가능
	if (INPUT->OnKeyDown(VK_LEFT) || INPUT->OnKeyDown(VK_RIGHT) ||
		INPUT->OnKeyDown(VK_UP) || INPUT->OnKeyDown(VK_DOWN))
		endCursor = 1 - endCursor;

	if (INPUT->OnKeyDown(VK_RETURN))
	{
		if (endCursor == 0)		// 계속 경영
		{
			if (ending == _EENDING::RICH)   achievedRich = true;
			if (ending == _EENDING::MASTERPIECE) achievedMaster = true;
			ending = _EENDING::NONE;
			view = _EVIEW::MAIN;
		}
		else
		{
			currentPhase = _EPhase::TITLE;
			select = _ESELECT::START;
		}
	}
}

// ===== 한 달 진행 (GDD 2장 (1)~(8)) =====
void TycoonContent::StartMonth()
{
	char buf[64];

	reportMsgs.clear();
	reportMoney.clear();
	monthRevenue = 0;
	raiseIdx = -1;
	snprintf(buf, sizeof(buf), "%d년 %d월 결산", year, month);
	reportTitle = buf;

	StepProject();							// (1) 개발 진행
	if (proj.active && proj.progress >= proj.need)
		DoRelease(false);					// (2) 완성작 출시
	StepSales();							// (3) 판매 정산
	RollApplicants();						// (4) 지원자 + 이벤트
	StepEvent();

	if (raiseIdx >= 0)						// 월급 인상 요구 -> 선택 화면
	{
		eventCursor = 0;
		view = _EVIEW::EVENT;
		return;
	}
	FinishMonth();
}

void TycoonContent::FinishMonth()
{
	if (month % 3 == 0)						// 분기마다 유행 재추첨
		UpdateTrend(true);

	int sal = TotalSalary();				// (5) 고정비
	if (sal > 0)
	{
		MoneyLine ml; ml.label = "직원 월급"; ml.amount = -sal;
		reportMoney.push_back(ml);
	}
	MoneyLine rent; rent.label = "사무실 임대료"; rent.amount = -OFFICE_RENT;
	reportMoney.push_back(rent);

	for (size_t i = 0; i < staffList.size(); i++)
		staffList[i].months++;

	int net = 0;							// (6) 결산 반영
	for (size_t i = 0; i < reportMoney.size(); i++)
		net += reportMoney[i].amount;
	reportBefore = money;
	reportNet = net;
	money += net;

	CheckEnding();							// (7) 파산/승리 판정

	month++;								// (8) 달력 진행
	if (month > 12) { month = 1; year++; }

	view = _EVIEW::REPORT;
}

void TycoonContent::StepProject()
{
	if (!proj.active)
		return;
	proj.progress += BASE_PROGRESS + DevSum();
	proj.quality += DevSum();
	proj.fun += CreSum() + rand() % 3;
	proj.devMonths++;
}

void TycoonContent::StepSales()
{
	int sum = 0;
	for (size_t i = 0; i < shelf.size(); i++)
	{
		Release& r = shelf[i];
		if (r.monthSales <= 0)
			continue;
		sum += r.monthSales / 2 * SCALE_PROFIT[r.scale];
		r.totalSales += r.monthSales;
		r.monthsOnSale++;
		r.monthSales /= 2;
		if (r.monthSales < MIN_SALES || r.monthsOnSale >= MAX_SALE_MON)
			r.monthSales = 0;				// 판매 종료
	}
	if (sum > 0)
	{
		MoneyLine ml; ml.label = "판매 수익"; ml.amount = sum;
		reportMoney.push_back(ml);
	}
	monthRevenue = sum;
}

void TycoonContent::StepEvent()
{
	if (rand() % 100 >= EVENT_CHANCE)
		return;

	for (int tries = 0; tries < 10; tries++)
	{
		int roll = rand() % 100;

		if (roll < 15)						// 정부 지원금
		{
			reportMsgs.push_back("[이벤트] 정부 인디게임 지원 사업 선정! (+500만원)");
			MoneyLine ml; ml.label = "이벤트: 정부 지원금"; ml.amount = 500;
			reportMoney.push_back(ml);
			return;
		}
		if (roll < 30)						// 게임쇼 호황
		{
			if (monthRevenue <= 0) continue;
			reportMsgs.push_back("[이벤트] 게임쇼 특수! 이번 달 판매 수익 2배!");
			MoneyLine ml; ml.label = "이벤트: 게임쇼 호황"; ml.amount = monthRevenue;
			reportMoney.push_back(ml);
			return;
		}
		if (roll < 50)						// 버그 사태
		{
			if (!proj.active || proj.quality < 5) continue;
			int loss = (proj.quality < 20) ? proj.quality : 20;
			proj.quality -= loss;
			char buf[96];
			snprintf(buf, sizeof(buf), "[이벤트] 치명적 버그 발견! \"%s\" 품질 -%d", proj.title.c_str(), loss);
			reportMsgs.push_back(buf);
			return;
		}
		if (roll < 70)						// 월급 인상 요구 (선택 화면으로)
		{
			int idx = -1;
			for (size_t i = 0; i < staffList.size(); i++)
				if (staffList[i].months >= RAISE_MONTHS) { idx = (int)i; break; }
			if (idx < 0) continue;
			raiseIdx = idx;
			return;
		}
		if (roll < 85)						// 악성 리뷰
		{
			if (fame <= 0) continue;
			fame -= 5;
			if (fame < 0) fame = 0;
			reportMsgs.push_back("[이벤트] 악성 리뷰가 퍼지고 있습니다... (명성 -5)");
			return;
		}
		// 스카우트 제의
		if ((int)applicants.size() >= MAX_APPLICANT) continue;
		{
			Staff s = MakeApplicant(true);
			applicants.push_back(s);
			char buf[96];
			snprintf(buf, sizeof(buf), "[이벤트] 능력자 [%s] 이(가) 지원! (개발 %d/창의 %d)", s.name.c_str(), s.dev, s.cre);
			reportMsgs.push_back(buf);
		}
		return;
	}
}

void TycoonContent::DoRelease(bool early)
{
	if (!proj.active)
		return;
	if (proj.progress >= proj.need)
		early = false;						// 다 만들었으면 조기 출시가 아니다

	Release rel;
	rel.title = proj.title;
	rel.genre = proj.genre;
	rel.scale = proj.scale;
	rel.rating = CalcRating(early);
	rel.monthSales = FirstMonthSales(rel);
	rel.totalSales = 0;
	rel.monthsOnSale = 0;

	// 명성 반영
	if (rel.rating >= 8.0f)      fame += 10;
	else if (rel.rating >= 6.0f) fame += 5;
	else if (rel.rating < 4.0f) { fame -= 5; if (fame < 0) fame = 0; }

	// 진열대가 꽉 차면 판매 종료작부터 밀어낸다
	if ((int)shelf.size() >= MAX_SHELF)
	{
		int drop = 0;
		for (size_t i = 0; i < shelf.size(); i++)
			if (shelf[i].monthSales == 0) { drop = (int)i; break; }
		shelf.erase(shelf.begin() + drop);
	}
	shelf.push_back(rel);
	if (rel.rating > bestRating)
		bestRating = rel.rating;
	proj.active = false;

	const char* verdict;
	if (rel.rating >= 9.0f)      verdict = "전설의 명작!!";
	else if (rel.rating >= 8.0f) verdict = "대박입니다!";
	else if (rel.rating >= 6.0f) verdict = "호평!";
	else if (rel.rating >= 4.0f) verdict = "그럭저럭...";
	else                         verdict = "혹평...";

	char buf[96];
	snprintf(buf, sizeof(buf), "출시! \"%s\" (%s/%s)%s", rel.title.c_str(),
		GENRE_NAME[rel.genre], SCALE_NAME[rel.scale], early ? " [조기 출시]" : "");
	reportMsgs.push_back(buf);
	snprintf(buf, sizeof(buf), "평점 %.1f / 10.0 - %s  (첫 달 %d장 예상)", rel.rating, verdict, rel.monthSales);
	reportMsgs.push_back(buf);
}

float TycoonContent::CalcRating(bool early) const
{
	int ratio = proj.progress * 100 / proj.need;
	if (ratio > 100) ratio = 100;

	int base = (proj.quality + proj.fun) * 100 / proj.need;		// 0~100 스케일
	if (early)
		base = base * ratio / 100;								// 조기 출시 페널티

	float r = base / 10.0f;
	if (proj.genre == trend)
		r += 0.5f;
	r += (rand() % 16 - 5) / 10.0f;								// -0.5 ~ +1.0
	if (r < 0.0f) r = 0.0f;
	if (r > 10.0f) r = 10.0f;
	return r;
}

int TycoonContent::FirstMonthSales(const Release& rel) const
{
	int sales = (int)(rel.rating * rel.rating * 100.0f);
	sales *= SCALE_SALE[rel.scale];
	sales = sales * (100 + fame) / 100;
	if (rel.genre == trend)
		sales = sales * 3 / 2;
	return sales;
}

void TycoonContent::RollApplicants()
{
	applicants.clear();
	int n = 1 + rand() % 2;
	for (int i = 0; i < n; i++)
		applicants.push_back(MakeApplicant(false));
}

TycoonContent::Staff TycoonContent::MakeApplicant(bool scout) const
{
	Staff s;
	s.name = STAFF_NAME[rand() % 16];
	if (scout)
	{
		s.dev = 8 + rand() % 3;
		s.cre = 8 + rand() % 3;
		s.salary = (s.dev + s.cre) * SALARY_RATE * 3 / 2;	// 몸값 1.5배
	}
	else
	{
		s.dev = 1 + rand() % 10;
		s.cre = 1 + rand() % 10;
		s.salary = (s.dev + s.cre) * SALARY_RATE;
	}
	s.months = 0;
	return s;
}

void TycoonContent::UpdateTrend(bool announce)
{
	trend = rand() % GENRE_N;
	if (announce)
	{
		char buf[64];
		snprintf(buf, sizeof(buf), "* 다음 분기 유행 장르는 [%s] 입니다!", GENRE_NAME[trend]);
		reportMsgs.push_back(buf);
	}
}

void TycoonContent::CheckEnding()
{
	if (money < 0)
	{
		ending = _EENDING::BANKRUPT;
		return;
	}
	if (!achievedMaster && bestRating >= 9.0f)
	{
		ending = _EENDING::MASTERPIECE;
		return;
	}
	if (!achievedRich && money >= WIN_MONEY)
	{
		ending = _EENDING::RICH;
		return;
	}
}

int TycoonContent::TotalSalary() const
{
	int sum = 0;
	for (size_t i = 0; i < staffList.size(); i++)
		sum += staffList[i].salary;
	return sum;
}

int TycoonContent::DevSum() const
{
	int sum = BOSS_DEV;						// 사장도 일한다
	for (size_t i = 0; i < staffList.size(); i++)
		sum += staffList[i].dev;
	return sum;
}

int TycoonContent::CreSum() const
{
	int sum = BOSS_CRE;
	for (size_t i = 0; i < staffList.size(); i++)
		sum += staffList[i].cre;
	return sum;
}

// ===== 인게임: 렌더 =====
void TycoonContent::OnInGameRender()
{
	switch (view)
	{
	case _EVIEW::MAIN:   RenderMain();   break;
	case _EVIEW::DEV:    RenderDev();    break;
	case _EVIEW::STAFF:  RenderStaff();  break;
	case _EVIEW::STATUS: RenderStatus(); break;
	case _EVIEW::SHELF:  RenderShelf();  break;
	case _EVIEW::EVENT:  RenderEvent();  break;
	case _EVIEW::REPORT: RenderReport(); break;
	case _EVIEW::ENDING: RenderEnding(); break;
	}
}

void TycoonContent::RenderHeader()
{
	char buf[96];
	snprintf(buf, sizeof(buf), "%s      %d년 %2d월      명성 %d", COMPANY_NAME, year, month, fame);
	SCREEN->OnDrawColor(3, 1, buf, WHITE);
	snprintf(buf, sizeof(buf), "자본금 %s만원   직원 %d/%d명   개발중 %d건   유행 [%s]",
		MoneyText(money).c_str(), (int)staffList.size(), MAX_STAFF,
		proj.active ? 1 : 0, GENRE_NAME[trend]);
	SCREEN->OnDrawColor(3, 2, buf, GRAY);
	SCREEN->OnDrawColor(2, 3, "----------------------------------------------------------------------------", DARKGRAY);
}

void TycoonContent::DrawMenuItem(short x, short y, const char* text, bool selected)
{
	if (selected)
	{
		SCREEN->OnDrawColor(x - 4, y, "▶", RED);
		SCREEN->OnDrawColor(x, y, text, YELLOW);
	}
	else
	{
		SCREEN->OnDrawColor(x, y, text, WHITE);
	}
}

void TycoonContent::DrawGauge(short x, short y, int val, int maxv, int width)
{
	char buf[64];
	int fill = (maxv > 0) ? val * width / maxv : 0;
	if (fill > width) fill = width;
	if (fill < 0) fill = 0;

	int o = 0;
	buf[o++] = '[';
	for (int i = 0; i < width; i++)
		buf[o++] = (i < fill) ? '#' : '-';
	buf[o++] = ']';
	buf[o] = '\0';
	SCREEN->OnDrawColor(x, y, buf, GREEN);
}

void TycoonContent::RenderMain()
{
	RenderHeader();

	SCREEN->OnDrawColor(3, 5, "목표: 자산 1억(10,000만원) 달성 or 평점 9.0 명작 출시", DARKGRAY);

	DrawMenuItem(30, 8, "게임 개발", mainCursor == 0);
	DrawMenuItem(30, 10, "직원 관리", mainCursor == 1);
	DrawMenuItem(30, 12, "회사 현황", mainCursor == 2);
	DrawMenuItem(30, 14, "출시작 목록", mainCursor == 3);
	DrawMenuItem(30, 16, "다음 달 진행 >>", mainCursor == 4);
	DrawMenuItem(30, 18, "타이틀로 나가기", mainCursor == 5);

	if (proj.active)
	{
		char buf[96];
		snprintf(buf, sizeof(buf), "개발중: \"%s\" %d/%d", proj.title.c_str(), proj.progress, proj.need);
		SCREEN->OnDrawColor(3, 21, buf, SKYBLUE);
	}
	SCREEN->OnDrawColor(3, 23, "위아래: 이동   Enter: 선택   ESC: 타이틀", GRAY);
}

void TycoonContent::RenderDev()
{
	RenderHeader();
	char buf[96];

	if (!proj.active)
	{
		SCREEN->OnDrawColor(3, 5, "[ 새 게임 기획 ]", YELLOW);

		snprintf(buf, sizeof(buf), "장르 :  < %s >%s", GENRE_NAME[pickGenre],
			(pickGenre == trend) ? "   (지금 유행!)" : "");
		DrawMenuItem(7, 8, buf, devCursor == 0);

		snprintf(buf, sizeof(buf), "규모 :  < %s >", SCALE_NAME[pickScale]);
		DrawMenuItem(7, 10, buf, devCursor == 1);
		snprintf(buf, sizeof(buf), "기획비 %d만원 / 필요 진행도 %d", SCALE_COST[pickScale], SCALE_NEED[pickScale]);
		SCREEN->OnDrawColor(11, 11, buf, DARKGRAY);

		DrawMenuItem(7, 14, "[ 개발 시작 ]  (제목은 자동으로 지어진다)", devCursor == 2);
		DrawMenuItem(7, 16, "[ 뒤로 ]", devCursor == 3);

		if (!uiMsg.empty())
			SCREEN->OnDrawColor(3, 20, uiMsg.c_str(), SKYBLUE);
		SCREEN->OnDrawColor(3, 23, "위아래: 항목   좌우: 변경   Enter: 확인   ESC: 뒤로", GRAY);
	}
	else
	{
		SCREEN->OnDrawColor(3, 5, "[ 개발 진행 상황 ]", YELLOW);

		snprintf(buf, sizeof(buf), "\"%s\"  (%s / %s)", proj.title.c_str(),
			GENRE_NAME[proj.genre], SCALE_NAME[proj.scale]);
		SCREEN->OnDrawColor(7, 7, buf, WHITE);

		SCREEN->OnDrawColor(7, 9, "진행도", GRAY);
		DrawGauge(15, 9, proj.progress, proj.need, 20);
		snprintf(buf, sizeof(buf), "%d / %d", proj.progress, proj.need);
		SCREEN->OnDrawColor(39, 9, buf, WHITE);

		snprintf(buf, sizeof(buf), "품질 %d   재미 %d   개발 %d개월차", proj.quality, proj.fun, proj.devMonths);
		SCREEN->OnDrawColor(7, 11, buf, WHITE);

		if (proj.progress >= proj.need)
			SCREEN->OnDrawColor(7, 13, "* 개발 완료! 다음 달 진행 시 출시됩니다.", GREEN);

		DrawMenuItem(7, 16, "[ 조기 출시 ]  (모자란 만큼 평점 감점)", devCursor == 0);
		DrawMenuItem(7, 18, "[ 뒤로 ]", devCursor == 1);

		if (earlyPending)
			SCREEN->OnDrawColor(3, 20, "! 한 번 더 Enter를 누르면 바로 출시합니다.", RED);
		SCREEN->OnDrawColor(3, 23, "위아래: 항목   Enter: 확인   ESC: 뒤로", GRAY);
	}
}

void TycoonContent::RenderStaff()
{
	RenderHeader();
	char buf[96];
	int staffN = (int)staffList.size();
	int appN = (int)applicants.size();

	snprintf(buf, sizeof(buf), "[ 직원 관리 ]   월급 합계 %d만원/월", TotalSalary());
	SCREEN->OnDrawColor(3, 5, buf, YELLOW);

	short y = 6;
	snprintf(buf, sizeof(buf), "-- 직원 (%d/%d) ------------------------------", staffN, MAX_STAFF);
	SCREEN->OnDrawColor(3, y++, buf, GRAY);
	if (staffN == 0)
		SCREEN->OnDrawColor(7, y++, "(직원 없음 - 사장 혼자 일하는 중)", DARKGRAY);
	for (int i = 0; i < staffN; i++)
	{
		snprintf(buf, sizeof(buf), "%-8s  개발 %2d  창의 %2d  월급 %4d만원  근속 %2d개월",
			staffList[i].name.c_str(), staffList[i].dev, staffList[i].cre,
			staffList[i].salary, staffList[i].months);
		DrawMenuItem(7, y++, buf, staffCursor == i);
	}

	snprintf(buf, sizeof(buf), "-- 이번 달 지원자 ----------------------------");
	SCREEN->OnDrawColor(3, y++, buf, GRAY);
	if (appN == 0)
		SCREEN->OnDrawColor(7, y++, "(지원자가 없습니다)", DARKGRAY);
	for (int i = 0; i < appN; i++)
	{
		snprintf(buf, sizeof(buf), "%-8s  개발 %2d  창의 %2d  희망 월급 %4d만원",
			applicants[i].name.c_str(), applicants[i].dev, applicants[i].cre,
			applicants[i].salary);
		DrawMenuItem(7, y++, buf, staffCursor == staffN + i);
	}

	DrawMenuItem(7, ++y, "[ 뒤로 ]", staffCursor == staffN + appN);

	if (firePending >= 0 && firePending < staffN)
	{
		snprintf(buf, sizeof(buf), "! 한 번 더 Enter: [%s] 해고 (퇴직금 %d만원)",
			staffList[firePending].name.c_str(), staffList[firePending].salary);
		SCREEN->OnDrawColor(3, 21, buf, RED);
	}
	else if (!uiMsg.empty())
	{
		SCREEN->OnDrawColor(3, 21, uiMsg.c_str(), SKYBLUE);
	}
	SCREEN->OnDrawColor(3, 23, "Enter: 직원=해고 / 지원자=고용   ESC: 뒤로", GRAY);
}

void TycoonContent::RenderStatus()
{
	RenderHeader();
	char buf[96];
	int totalSold = 0;
	for (size_t i = 0; i < shelf.size(); i++)
		totalSold += shelf[i].totalSales;
	int elapsed = (year - START_YEAR) * 12 + (month - 1);

	SCREEN->OnDrawColor(3, 5, "[ 회사 현황 ]", YELLOW);

	snprintf(buf, sizeof(buf), "자본금        %s만원", MoneyText(money).c_str());
	SCREEN->OnDrawColor(7, 7, buf, WHITE);
	snprintf(buf, sizeof(buf), "명성          %d", fame);
	SCREEN->OnDrawColor(7, 8, buf, WHITE);
	snprintf(buf, sizeof(buf), "창업          %d년 1월 (경과 %d개월)", START_YEAR, elapsed);
	SCREEN->OnDrawColor(7, 9, buf, WHITE);
	snprintf(buf, sizeof(buf), "직원          %d명 (월급 합계 %d만원)", (int)staffList.size(), TotalSalary());
	SCREEN->OnDrawColor(7, 10, buf, WHITE);
	snprintf(buf, sizeof(buf), "월 고정비     %d만원 (임대료 %d + 월급 %d)",
		OFFICE_RENT + TotalSalary(), OFFICE_RENT, TotalSalary());
	SCREEN->OnDrawColor(7, 11, buf, WHITE);
	snprintf(buf, sizeof(buf), "출시작        %d작품 (누적 판매 %d장)", (int)shelf.size(), totalSold);
	SCREEN->OnDrawColor(7, 12, buf, WHITE);
	snprintf(buf, sizeof(buf), "최고 평점     %.1f", bestRating);
	SCREEN->OnDrawColor(7, 13, buf, WHITE);

	SCREEN->OnDrawColor(7, 16, "승리: 자산 10,000만원 달성 or 평점 9.0 이상 명작 출시", DARKGRAY);
	SCREEN->OnDrawColor(7, 17, "패배: 자본금이 0 미만이 되면 파산", DARKGRAY);

	SCREEN->OnDrawColor(3, 23, "Enter/ESC: 뒤로", GRAY);
}

void TycoonContent::RenderShelf()
{
	RenderHeader();
	char buf[96];
	const int visible = 13;

	snprintf(buf, sizeof(buf), "[ 출시작 목록 ]   총 %d작품", (int)shelf.size());
	SCREEN->OnDrawColor(3, 5, buf, YELLOW);

	if (shelf.empty())
	{
		SCREEN->OnDrawColor(7, 8, "(아직 출시한 게임이 없습니다)", DARKGRAY);
	}
	else
	{
		short y = 7;
		for (int i = shelfScroll; i < (int)shelf.size() && i < shelfScroll + visible; i++)
		{
			const Release& r = shelf[i];
			snprintf(buf, sizeof(buf), "%2d) %-14s %s/%s  평점 %4.1f  누적 %6d장  %s",
				i + 1, r.title.c_str(), GENRE_NAME[r.genre], SCALE_NAME[r.scale],
				r.rating, r.totalSales, (r.monthSales > 0) ? "판매중" : "판매종료");
			SCREEN->OnDrawColor(5, y++, buf, (r.monthSales > 0) ? WHITE : DARKGRAY);
		}
		if ((int)shelf.size() > visible)
		{
			snprintf(buf, sizeof(buf), "( %d ~ %d / %d )", shelfScroll + 1,
				shelfScroll + visible < (int)shelf.size() ? shelfScroll + visible : (int)shelf.size(),
				(int)shelf.size());
			SCREEN->OnDrawColor(3, 21, buf, GRAY);
		}
	}
	SCREEN->OnDrawColor(3, 23, "위아래: 스크롤   Enter/ESC: 뒤로", GRAY);
}

void TycoonContent::RenderEvent()
{
	RenderHeader();
	char buf[96];

	SCREEN->OnDrawColor(3, 6, "[ 돌발 이벤트 ]", RED);

	if (raiseIdx >= 0 && raiseIdx < (int)staffList.size())
	{
		const Staff& s = staffList[raiseIdx];
		snprintf(buf, sizeof(buf), "[%s] 이(가) 월급 인상을 요구합니다!", s.name.c_str());
		SCREEN->OnDrawColor(7, 9, buf, WHITE);
		snprintf(buf, sizeof(buf), "월급 %d만원  ->  %d만원 (+20%%)", s.salary, s.salary * 12 / 10);
		SCREEN->OnDrawColor(7, 11, buf, YELLOW);
		SCREEN->OnDrawColor(7, 13, "거절하면 퇴사할 것 같습니다...", DARKGRAY);
	}

	DrawMenuItem(26, 16, "[ 수락 ]", eventCursor == 0);
	DrawMenuItem(46, 16, "[ 거절 ]", eventCursor == 1);

	SCREEN->OnDrawColor(3, 23, "좌우: 선택   Enter: 확인", GRAY);
}

void TycoonContent::RenderReport()
{
	RenderHeader();
	char buf[96];

	snprintf(buf, sizeof(buf), "[ %s ]", reportTitle.c_str());
	SCREEN->OnDrawColor(3, 5, buf, YELLOW);

	short y = 7;
	int msgN = (int)reportMsgs.size();
	if (msgN > 6) msgN = 6;
	for (int i = 0; i < msgN; i++)
		SCREEN->OnDrawColor(5, y++, reportMsgs[i].c_str(), SKYBLUE);
	if (msgN > 0)
		y++;

	if (!reportMoney.empty())
	{
		int lineN = (int)reportMoney.size();
		if (lineN > 6) lineN = 6;
		for (int i = 0; i < lineN; i++)
		{
			const MoneyLine& ml = reportMoney[i];
			snprintf(buf, sizeof(buf), "%-22s %s%s만원", ml.label.c_str(),
				(ml.amount >= 0) ? "+" : "", MoneyText(ml.amount).c_str());
			SCREEN->OnDrawColor(7, y++, buf, (ml.amount >= 0) ? GREEN : RED);
		}
		SCREEN->OnDrawColor(7, y++, "--------------------------------------", DARKGRAY);
		snprintf(buf, sizeof(buf), "순이익 %s%s만원", (reportNet >= 0) ? "+" : "", MoneyText(reportNet).c_str());
		SCREEN->OnDrawColor(7, y++, buf, YELLOW);
		snprintf(buf, sizeof(buf), "자본금 %s만원 -> %s만원",
			MoneyText(reportBefore).c_str(), MoneyText(reportBefore + reportNet).c_str());
		SCREEN->OnDrawColor(7, y++, buf, WHITE);
	}

	SCREEN->OnDrawColor(3, 23, "Enter: 계속", GRAY);
}

void TycoonContent::RenderEnding()
{
	char buf[96];
	int totalSold = 0;
	for (size_t i = 0; i < shelf.size(); i++)
		totalSold += shelf[i].totalSales;
	int elapsed = (year - START_YEAR) * 12 + (month - 1);

	SCREEN->OnDrawColor(14, 4, "==================================================", DARKGRAY);
	switch (ending)
	{
	case _EENDING::BANKRUPT:
		SCREEN->OnDrawColor(24, 6, "파 산 ...  회사가 문을 닫았습니다.", RED);
		break;
	case _EENDING::RICH:
		SCREEN->OnDrawColor(20, 6, "거상 엔딩!  자산 1억을 달성했습니다!", YELLOW);
		break;
	case _EENDING::MASTERPIECE:
		SCREEN->OnDrawColor(18, 6, "명작 엔딩!  게임 역사에 이름을 남겼습니다!", YELLOW);
		break;
	default:
		break;
	}
	SCREEN->OnDrawColor(14, 8, "==================================================", DARKGRAY);

	snprintf(buf, sizeof(buf), "경영 기간   %d개월 (%d년 %d월)", elapsed, year, month);
	SCREEN->OnDrawColor(24, 11, buf, WHITE);
	snprintf(buf, sizeof(buf), "최종 자본   %s만원", MoneyText(money).c_str());
	SCREEN->OnDrawColor(24, 12, buf, WHITE);
	snprintf(buf, sizeof(buf), "출시작      %d작품 / 누적 판매 %d장", (int)shelf.size(), totalSold);
	SCREEN->OnDrawColor(24, 13, buf, WHITE);
	snprintf(buf, sizeof(buf), "최고 평점   %.1f", bestRating);
	SCREEN->OnDrawColor(24, 14, buf, WHITE);
	snprintf(buf, sizeof(buf), "명성        %d", fame);
	SCREEN->OnDrawColor(24, 15, buf, WHITE);

	if (ending == _EENDING::BANKRUPT)
	{
		SCREEN->OnDrawColor(26, 19, "Enter: 타이틀로", GRAY);
	}
	else
	{
		DrawMenuItem(20, 19, "[ 계속 경영하기 ]", endCursor == 0);
		DrawMenuItem(46, 19, "[ 타이틀로 ]", endCursor == 1);
		SCREEN->OnDrawColor(24, 22, "좌우: 선택   Enter: 확인", GRAY);
	}
}

// ===== 유틸 =====
std::string TycoonContent::MoneyText(int v) const
{
	char digits[16];
	char out[24];
	int neg = 0;
	if (v < 0) { neg = 1; v = -v; }
	snprintf(digits, sizeof(digits), "%d", v);
	int n = (int)strlen(digits);
	int o = 0;
	for (int i = 0; i < n; i++)
	{
		if (i > 0 && (n - i) % 3 == 0)
			out[o++] = ',';
		out[o++] = digits[i];
	}
	out[o] = '\0';
	return std::string(neg ? "-" : "") + out;
}

std::string TycoonContent::MakeTitle() const
{
	return std::string(TITLE_HEAD[rand() % 10]) + " " + TITLE_TAIL[rand() % 10];
}
