#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <stdlib.h>
#include "market.h"
#include "project.h"
#include "employee.h"
#include "ui.h"

static int s_monthRevenue;      /* 이번 달 판매 수익 합계 (호황 이벤트용) */

/* 매월: 판매 중인 게임들 정산 후 판매량 반감 (GDD 5.6) */
void UpdateSales(Company *c)
{
    int i, revenue;
    char label[64];

    s_monthRevenue = 0;
    for (i = 0; i < c->shelfCount; i++) {
        Release *r = &c->shelf[i];
        if (r->monthSales <= 0)
            continue;
        revenue = r->monthSales / 2 * SCALE_PROFIT_MULT[r->scale];
        r->totalSales += r->monthSales;
        r->monthsOnSale++;
        snprintf(label, sizeof(label), "판매 수익 \"%s\"", r->title);
        Report(label, revenue);
        s_monthRevenue += revenue;

        r->monthSales /= 2;
        if (r->monthSales < MIN_SALES || r->monthsOnSale >= MAX_SALE_MONTHS)
            r->monthSales = 0;              /* 판매 종료 */
    }
}

/* 매월 30% 확률로 이벤트 1건 (GDD 5.7) - 조건이 안 맞으면 다시 뽑는다 */
void MonthlyEvent(Company *c)
{
    int tries, roll, i, idx, loss, newSalary;

    if (rand() % 100 >= EVENT_CHANCE)
        return;

    for (tries = 0; tries < 10; tries++) {
        roll = rand() % 100;

        if (roll < 15) {                    /* 정부 지원금 */
            printf("\n  [이벤트] 정부 인디게임 지원 사업에 선정됐습니다! (+500만원)\n");
            Report("이벤트: 정부 지원금", 500);
            return;
        }
        if (roll < 30) {                    /* 게임쇼 호황 */
            if (s_monthRevenue <= 0)
                continue;
            printf("\n  [이벤트] 게임쇼 특수! 이번 달 판매 수익이 2배가 됐습니다.\n");
            Report("이벤트: 게임쇼 호황(판매 2배)", s_monthRevenue);
            return;
        }
        if (roll < 50) {                    /* 버그 사태 */
            if (!c->proj.active || c->proj.quality < 5)
                continue;
            loss = 20;
            if (c->proj.quality < loss)
                loss = c->proj.quality;
            c->proj.quality -= loss;
            printf("\n  [이벤트] 치명적 버그 발견! \"%s\" 품질 -%d\n",
                   c->proj.title, loss);
            return;
        }
        if (roll < 70) {                    /* 월급 인상 요구 */
            idx = -1;
            for (i = 0; i < c->staffCount; i++)
                if (c->staff[i].months >= RAISE_MONTHS) { idx = i; break; }
            if (idx < 0)
                continue;
            newSalary = c->staff[idx].salary * 12 / 10;
            printf("\n  [이벤트] [%s] 이(가) 월급 인상을 요구합니다! (%d -> %d만원)\n",
                   c->staff[idx].name, c->staff[idx].salary, newSalary);
            if (ReadInt("  [1] 수락   [2] 거절 : ", 1, 2) == 1) {
                c->staff[idx].salary = newSalary;
                c->staff[idx].months = 0;   /* 당분간 다시 요구하지 않는다 */
                printf("  * 월급을 올려줬습니다.\n");
            } else {
                printf("  * [%s] 이(가) 회사를 떠났습니다...\n", c->staff[idx].name);
                QuitEmployee(c, idx);
            }
            return;
        }
        if (roll < 85) {                    /* 악성 리뷰 */
            if (c->fame <= 0)
                continue;
            c->fame -= 5;
            if (c->fame < 0) c->fame = 0;
            printf("\n  [이벤트] 악성 리뷰가 퍼지고 있습니다... (명성 -5)\n");
            return;
        }
        /* 스카우트 제의 */
        printf("\n  [이벤트] 스카우트 소식!\n");
        AddScoutApplicant(c);
        return;
    }
}

void UpdateTrend(Company *c, int announce)
{
    c->trend = (Genre)(rand() % GENRE_COUNT);
    if (announce)
        printf("\n  * 시장 조사: 다음 분기 유행 장르는 [%s] 입니다!\n",
               GenreName((int)c->trend));
}

void ReleaseList(const Company *c)
{
    int i;
    ClearScreen();
    PrintHeader(c);
    printf("  [ 출시작 목록 ]  총 %d작품\n\n", c->shelfCount);
    if (c->shelfCount == 0) {
        printf("   (아직 출시한 게임이 없습니다)\n");
    } else {
        for (i = 0; i < c->shelfCount; i++) {
            const Release *r = &c->shelf[i];
            printf("   %2d) %-14s %-6s/%s  평점 %4.1f  누적 %6d장  %s\n",
                   i + 1, r->title, GenreName((int)r->genre),
                   ScaleName((int)r->scale), r->rating, r->totalSales,
                   r->monthSales > 0 ? "판매중" : "판매종료");
        }
    }
    PauseEnter();
}
