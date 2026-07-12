#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "company.h"
#include "employee.h"
#include "project.h"
#include "market.h"
#include "ui.h"

/* ===== 월 결산 리포트 버퍼 ===== */
#define MAX_REPORT 24
typedef struct {
    char label[64];
    int  amount;
} ReportLine;

static ReportLine s_rep[MAX_REPORT];
static int s_repCount;

void ResetReport(void)
{
    s_repCount = 0;
}

void Report(const char *label, int amount)
{
    if (s_repCount >= MAX_REPORT)
        return;
    snprintf(s_rep[s_repCount].label, sizeof(s_rep[s_repCount].label), "%s", label);
    s_rep[s_repCount].amount = amount;
    s_repCount++;
}

/* ===== 회사 생명주기 ===== */
void InitCompany(Company *c, const char *name)
{
    memset(c, 0, sizeof(*c));
    snprintf(c->name, sizeof(c->name), "%s", name);
    c->money = START_MONEY;
    c->year = START_YEAR;
    c->month = 1;
    c->staff = (Employee *)malloc(sizeof(Employee) * START_STAFF_CAP);
    if (c->staff == NULL) {
        printf("메모리 할당 실패\n");
        exit(1);
    }
    c->staffCap = START_STAFF_CAP;
    UpdateTrend(c, 0);
    RollApplicants(c);
}

void FreeCompany(Company *c)
{
    free(c->staff);
    c->staff = NULL;
    c->staffCount = c->staffCap = 0;
}

int TotalMonths(const Company *c)
{
    return (c->year - START_YEAR) * 12 + (c->month - 1);
}

/* ===== 월 결산 출력 + 자본금 반영 ===== */
static void PrintMonthlyReport(Company *c)
{
    int i, net = 0;
    char m[24], before[24], after[24];

    printf("\n  ============ %d년 %d월 결산 ============\n", c->year, c->month);
    if (s_repCount == 0)
        printf("   (수입/지출 내역 없음)\n");
    for (i = 0; i < s_repCount; i++) {
        MoneyStr(s_rep[i].amount, m, sizeof(m));
        printf("   %-30s %s%s만원\n",
               s_rep[i].label, s_rep[i].amount >= 0 ? "+" : "", m);
        net += s_rep[i].amount;
    }
    printf("   ----------------------------------------\n");
    MoneyStr(net, m, sizeof(m));
    MoneyStr(c->money, before, sizeof(before));
    MoneyStr(c->money + net, after, sizeof(after));
    printf("   순이익 %s%s만원\n", net >= 0 ? "+" : "", m);
    printf("   자본금 %s만원 -> %s만원\n", before, after);
    printf("  ==========================================\n");
    c->money += net;
}

static void CheckEnding(Company *c)
{
    if (c->money < 0) {
        c->ending = END_BANKRUPT;
        return;
    }
    if (!c->achievedMasterpiece && c->bestRating >= MASTERPIECE_RATING) {
        c->ending = END_MASTERPIECE;
        return;
    }
    if (!c->achievedRich && c->money >= WIN_MONEY) {
        c->ending = END_RICH;
        return;
    }
}

/* ===== 한 달 진행 (GDD 2장의 (1)~(8) 순서 고정) ===== */
void AdvanceMonth(Company *c)
{
    int salary;

    ClearScreen();
    printf("\n  %d년 %d월이 지나갑니다...\n", c->year, c->month);
    ResetReport();

    ProgressProject(c);                     /* (1) 개발 진행 */
    if (ProjectDone(c))                     /* (2) 완성작 출시 */
        ReleaseNow(c, 0);
    UpdateSales(c);                         /* (3) 판매 정산 */
    RollApplicants(c);                      /* (4) 다음 달 지원자 + 이벤트 */
    MonthlyEvent(c);
    if (c->month % 3 == 0)                  /*     분기마다 유행 재추첨 */
        UpdateTrend(c, 1);
    salary = TotalSalary(c);                /* (5) 고정비 지출 */
    if (salary > 0)
        Report("직원 월급", -salary);
    Report("사무실 임대료", -OFFICE_RENT);
    AgeStaff(c);
    PrintMonthlyReport(c);                  /* (6) 결산 리포트 */
    CheckEnding(c);                         /* (7) 파산/승리 판정 */
    c->month++;                             /* (8) 달력 진행 */
    if (c->month > 12) {
        c->month = 1;
        c->year++;
    }
    PauseEnter();
}

void StatusScreen(const Company *c)
{
    char m[24];
    int i, totalSold = 0;

    for (i = 0; i < c->shelfCount; i++)
        totalSold += c->shelf[i].totalSales;

    ClearScreen();
    PrintHeader(c);
    printf("  [ 회사 현황 ]\n\n");
    MoneyStr(c->money, m, sizeof(m));
    printf("   자본금        %s만원\n", m);
    printf("   명성          %d\n", c->fame);
    printf("   창업          %d년 1월 (경과 %d개월)\n", START_YEAR, TotalMonths(c));
    printf("   직원          %d명 (월급 합계 %d만원)\n", c->staffCount, TotalSalary(c));
    printf("   월 고정비     %d만원 (임대료 %d + 월급 %d)\n",
           OFFICE_RENT + TotalSalary(c), OFFICE_RENT, TotalSalary(c));
    printf("   출시작        %d작품 (누적 판매 %d장)\n", c->shelfCount, totalSold);
    printf("   최고 평점     %.1f\n", c->bestRating);
    printf("   유행 장르     %s\n", GenreName((int)c->trend));
    printf("\n   승리: 자산 %d만원 달성 또는 평점 %.1f 이상 명작 출시\n",
           WIN_MONEY, MASTERPIECE_RATING);
    printf("   패배: 자본금이 0 미만이 되면 파산\n");
    PauseEnter();
}
