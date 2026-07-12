#pragma once
#include "Interface/IGameContent.h"
#include <string>
#include <vector>

// GameDev Tycoon - run a game studio, month by month (turn-based scene).
// Design doc: Docs/GameDevTycoon_GDD.md
class TycoonContent : public IGameContent
{
private:
	// ===== balance constants (tune here) =====
	static const int START_YEAR    = 2026;
	static const int START_MONEY   = 5000;		// x 10,000 KRW
	static const int OFFICE_RENT   = 100;
	static const int WIN_MONEY     = 10000;
	static const int BOSS_DEV      = 3;			// the player works too
	static const int BOSS_CRE      = 3;
	static const int SALARY_RATE   = 30;		// salary = (dev+cre) * 30
	static const int BASE_PROGRESS = 15;		// monthly progress = 15 + sum(dev)
	static const int MAX_STAFF     = 8;			// office is full at 8
	static const int MAX_APPLICANT = 3;
	static const int MAX_SHELF     = 50;
	static const int MAX_SALE_MON  = 6;
	static const int MIN_SALES     = 100;
	static const int EVENT_CHANCE  = 30;		// % per month
	static const int RAISE_MONTHS  = 6;

	enum class _ESELECT { NONE, START, EXIT, END };

	// ingame sub screens
	enum class _EVIEW { MAIN, DEV, STAFF, STATUS, SHELF, EVENT, REPORT, ENDING };
	enum class _EENDING { NONE, BANKRUPT, RICH, MASTERPIECE };

	struct Staff {
		std::string name;
		int dev, cre;			// 1~10
		int salary;				// per month
		int months;				// served
	};

	struct Project {
		std::string title;
		int genre, scale;
		int progress, need;
		int quality, fun;
		int devMonths;
		bool active;
	};

	struct Release {
		std::string title;
		int genre, scale;
		float rating;			// 0.0 ~ 10.0
		int monthSales;			// 0 = off sale
		int totalSales;
		int monthsOnSale;
	};

	struct MoneyLine {
		std::string label;
		int amount;
	};

	// ----- title phase -----
	_ESELECT select = _ESELECT::START;

	// ----- company state -----
	_EVIEW view = _EVIEW::MAIN;
	int money = 0, fame = 0;
	int year = 0, month = 0;
	int trend = 0;				// genre index in fashion
	std::vector<Staff> staffList;
	std::vector<Staff> applicants;
	Project proj;
	std::vector<Release> shelf;
	_EENDING ending = _EENDING::NONE;
	bool achievedRich = false, achievedMaster = false;
	float bestRating = 0.0f;

	// ----- ui state -----
	int mainCursor = 0;
	int devCursor = 0;			// idle: 0 genre 1 scale 2 start 3 back / active: 0 early 1 back
	int pickGenre = 0, pickScale = 0;
	bool earlyPending = false;	// press Enter twice to early-release
	int staffCursor = 0;
	int firePending = -1;		// press Enter twice to fire
	int shelfScroll = 0;
	int eventCursor = 0;		// raise demand: 0 accept 1 reject
	int endCursor = 0;			// victory: 0 continue 1 to title
	std::string uiMsg;

	// ----- month result -----
	std::vector<std::string> reportMsgs;
	std::vector<MoneyLine> reportMoney;
	std::string reportTitle;
	int reportNet = 0;
	int reportBefore = 0;
	int monthRevenue = 0;		// sales revenue this month (for boom event)
	int raiseIdx = -1;			// staff index demanding a raise

public:
	virtual void OnInit();
	virtual void OnRelease();

	virtual void OnTitleUpdate();
	virtual void OnTitleRender();

	virtual void OnInGameUpdate();
	virtual void OnInGameRender();

private:
	void NewGame();

	// month flow: StartMonth -> (EVENT view?) -> FinishMonth -> REPORT view
	void StartMonth();
	void FinishMonth();
	void StepProject();
	void StepSales();
	void StepEvent();
	void DoRelease(bool early);
	float CalcRating(bool early) const;
	int FirstMonthSales(const Release& rel) const;
	void RollApplicants();
	void UpdateTrend(bool announce);
	void CheckEnding();

	int TotalSalary() const;
	int DevSum() const;
	int CreSum() const;

	// per-view update / render
	void UpdateMain();   void RenderMain();
	void UpdateDev();    void RenderDev();
	void UpdateStaff();  void RenderStaff();
	void UpdateStatus(); void RenderStatus();
	void UpdateShelf();  void RenderShelf();
	void UpdateEvent();  void RenderEvent();
	void UpdateReport(); void RenderReport();
	void UpdateEnding(); void RenderEnding();
	void RenderHeader();

	// utils
	std::string MoneyText(int v) const;
	std::string MakeTitle() const;
	Staff MakeApplicant(bool scout) const;
	void DrawMenuItem(short x, short y, const char* text, bool selected);
	void DrawGauge(short x, short y, int val, int maxv, int width);
};
