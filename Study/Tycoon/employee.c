#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "employee.h"
#include "ui.h"

static const char *NAME_POOL[] = {
    "김개발", "이코딩", "박버그", "최야근", "정디버그", "한알고",
    "오포인터", "유메모리", "신렌더", "장서버", "임클라", "송기획",
    "백그래픽", "문사운드", "양테스트", "황빌드"
};
#define NAME_POOL_N ((int)(sizeof(NAME_POOL) / sizeof(NAME_POOL[0])))

static void RandomEmployee(Employee *e)
{
    strcpy(e->name, NAME_POOL[rand() % NAME_POOL_N]);
    e->dev = 1 + rand() % 10;
    e->cre = 1 + rand() % 10;
    e->salary = (e->dev + e->cre) * SALARY_RATE;
    e->months = 0;
}

void RollApplicants(Company *c)
{
    int i;
    c->applicantCount = 1 + rand() % 2;
    for (i = 0; i < c->applicantCount; i++)
        RandomEmployee(&c->applicants[i]);
}

void AddScoutApplicant(Company *c)
{
    Employee e;
    strcpy(e.name, NAME_POOL[rand() % NAME_POOL_N]);
    e.dev = 8 + rand() % 3;
    e.cre = 8 + rand() % 3;
    e.salary = (e.dev + e.cre) * SALARY_RATE * 3 / 2;   /* 몸값 1.5배 */
    e.months = 0;
    if (c->applicantCount < MAX_APPLICANTS)
        c->applicants[c->applicantCount++] = e;
    else
        c->applicants[0] = e;
    printf("  * 능력자 [%s] (개발 %d/창의 %d) 이(가) 이직 의사를 밝혔습니다! (월급 %d만원)\n",
           e.name, e.dev, e.cre, e.salary);
}

int TotalSalary(const Company *c)
{
    int sum = 0, i;
    for (i = 0; i < c->staffCount; i++)
        sum += c->staff[i].salary;
    return sum;
}

int DevSum(const Company *c)
{
    int sum = BOSS_DEV, i;                  /* 사장도 일한다 */
    for (i = 0; i < c->staffCount; i++)
        sum += c->staff[i].dev;
    return sum;
}

int CreSum(const Company *c)
{
    int sum = BOSS_CRE, i;
    for (i = 0; i < c->staffCount; i++)
        sum += c->staff[i].cre;
    return sum;
}

void AgeStaff(Company *c)
{
    int i;
    for (i = 0; i < c->staffCount; i++)
        c->staff[i].months++;
}

void QuitEmployee(Company *c, int idx)
{
    int i;
    for (i = idx; i < c->staffCount - 1; i++)
        c->staff[i] = c->staff[i + 1];
    c->staffCount--;
}

/* 슬롯이 꽉 차면 realloc으로 확장 (11장: 고정 배열의 한계를 푼다) */
static void Hire(Company *c, const Employee *e)
{
    if (c->staffCount == c->staffCap) {
        int newCap = c->staffCap * 2;
        Employee *tmp = (Employee *)realloc(c->staff, sizeof(Employee) * newCap);
        if (tmp == NULL) {
            printf("  ! 메모리가 부족해 사무실을 확장할 수 없습니다.\n");
            return;
        }
        c->staff = tmp;
        c->staffCap = newCap;
        printf("  * 사무실을 확장했습니다! (최대 %d명)\n", newCap);
    }
    c->staff[c->staffCount++] = *e;
    printf("  * [%s] 을(를) 고용했습니다. (월급 %d만원)\n", e->name, e->salary);
}

static void PrintStaffList(const Company *c)
{
    int i;
    printf("  -- 직원 명단 (%d명 / 자리 %d) -------------------\n",
           c->staffCount, c->staffCap);
    if (c->staffCount == 0)
        printf("   (직원이 없습니다 - 사장 혼자 일하는 중)\n");
    for (i = 0; i < c->staffCount; i++)
        printf("   %d) %-10s 개발 %2d  창의 %2d  월급 %4d만원  근속 %d개월\n",
               i + 1, c->staff[i].name, c->staff[i].dev, c->staff[i].cre,
               c->staff[i].salary, c->staff[i].months);
}

void StaffMenu(Company *c)
{
    int i, sel, n, ok;
    for (;;) {
        ClearScreen();
        PrintHeader(c);
        printf("  [ 직원 관리 ]   월급 합계 %d만원/월\n\n", TotalSalary(c));
        PrintStaffList(c);
        printf("\n  -- 이번 달 지원자 ------------------------------\n");
        if (c->applicantCount == 0)
            printf("   (지원자가 없습니다)\n");
        for (i = 0; i < c->applicantCount; i++)
            printf("   %d) %-10s 개발 %2d  창의 %2d  희망 월급 %4d만원\n",
                   i + 1, c->applicants[i].name, c->applicants[i].dev,
                   c->applicants[i].cre, c->applicants[i].salary);
        printf("--------------------------------------------------\n");
        printf("  [1] 고용   [2] 해고   [0] 뒤로\n");
        sel = ReadInt("  >> 선택: ", 0, 2);

        if (sel == 0)
            return;

        if (sel == 1) {
            if (c->applicantCount == 0) {
                printf("  ! 지원자가 없습니다. 다음 달을 기다려 보세요.\n");
                PauseEnter();
                continue;
            }
            n = ReadInt("  >> 몇 번 지원자를 고용할까요? (0=취소): ", 0, c->applicantCount);
            if (n == 0)
                continue;
            Hire(c, &c->applicants[n - 1]);
            for (i = n - 1; i < c->applicantCount - 1; i++)
                c->applicants[i] = c->applicants[i + 1];
            c->applicantCount--;
            PauseEnter();
        } else {
            if (c->staffCount == 0) {
                printf("  ! 해고할 직원이 없습니다.\n");
                PauseEnter();
                continue;
            }
            n = ReadInt("  >> 몇 번 직원을 해고할까요? (0=취소): ", 0, c->staffCount);
            if (n == 0)
                continue;
            printf("  퇴직금 %d만원(월급 1개월분)을 지급해야 합니다.\n",
                   c->staff[n - 1].salary);
            ok = ReadInt("  [1] 해고 확정   [0] 취소 : ", 0, 1);
            if (ok == 1) {
                c->money -= c->staff[n - 1].salary;
                printf("  * [%s] 을(를) 해고했습니다.\n", c->staff[n - 1].name);
                QuitEmployee(c, n - 1);
            }
            PauseEnter();
        }
    }
}
