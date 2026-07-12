#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "project.h"
#include "employee.h"
#include "ui.h"

/*                                          소형  중형  대형 */
const int SCALE_COST[SCALE_COUNT]        = { 200,  500, 1200 };
const int SCALE_NEED[SCALE_COUNT]        = { 100,  200,  400 };
const int SCALE_SALE_MULT[SCALE_COUNT]   = {   1,    2,    4 };
const int SCALE_PROFIT_MULT[SCALE_COUNT] = {   1,    2,    3 };

int ProjectDone(const Company *c)
{
    return c->proj.active && c->proj.progress >= c->proj.need;
}

/* 매월: 진행도/품질/재미 누적 (GDD 5.4) */
void ProgressProject(Company *c)
{
    Project *p = &c->proj;
    if (!p->active)
        return;
    p->progress += BASE_PROGRESS + DevSum(c);
    p->quality  += DevSum(c);
    p->fun      += CreSum(c) + rand() % 3;
    p->devMonths++;
}

/* 평점 = 품질·재미 밀도 + 유행 보정 + 랜덤 (GDD 5.5) */
static float CalcRating(const Company *c, int early)
{
    const Project *p = &c->proj;
    int ratio = p->progress * 100 / p->need;
    int base;
    float r;

    if (ratio > 100) ratio = 100;
    base = (p->quality + p->fun) * 100 / p->need;   /* 0~100 스케일 */
    if (early)
        base = base * ratio / 100;                  /* 조기 출시 페널티 */
    r = base / 10.0f;
    if (p->genre == c->trend)
        r += 0.5f;
    r += (rand() % 16 - 5) / 10.0f;                 /* -0.5 ~ +1.0 */
    if (r < 0.0f) r = 0.0f;
    if (r > 10.0f) r = 10.0f;
    return r;
}

/* 첫 달 판매량 (GDD 5.6) - 이후는 market.c에서 매달 반감 */
static int FirstMonthSales(const Company *c, const Release *rel)
{
    int sales = (int)(rel->rating * rel->rating * 100.0f);
    sales *= SCALE_SALE_MULT[rel->scale];
    sales = sales * (100 + c->fame) / 100;
    if (rel->genre == c->trend)
        sales = sales * 3 / 2;
    return sales;
}

void ReleaseNow(Company *c, int early)
{
    Project *p = &c->proj;
    Release rel;
    const char *verdict;
    int i;

    if (!p->active)
        return;
    if (p->progress >= p->need)
        early = 0;                          /* 다 만들었으면 조기 출시가 아니다 */

    memset(&rel, 0, sizeof(rel));
    strcpy(rel.title, p->title);
    rel.genre = p->genre;
    rel.scale = p->scale;
    rel.rating = CalcRating(c, early);
    rel.monthSales = FirstMonthSales(c, &rel);

    /* 명성 반영 (GDD 5.5) */
    if (rel.rating >= 8.0f)      c->fame += 10;
    else if (rel.rating >= 6.0f) c->fame += 5;
    else if (rel.rating < 4.0f) {
        c->fame -= 5;
        if (c->fame < 0) c->fame = 0;
    }

    /* 진열대가 꽉 차면 판매 종료작부터 밀어낸다 */
    if (c->shelfCount == MAX_SHELF) {
        int drop = 0;
        for (i = 0; i < c->shelfCount; i++)
            if (c->shelf[i].monthSales == 0) { drop = i; break; }
        for (i = drop; i < c->shelfCount - 1; i++)
            c->shelf[i] = c->shelf[i + 1];
        c->shelfCount--;
    }
    c->shelf[c->shelfCount++] = rel;
    if (rel.rating > c->bestRating)
        c->bestRating = rel.rating;
    p->active = 0;

    if (rel.rating >= 9.0f)      verdict = "전설의 명작!!";
    else if (rel.rating >= 8.0f) verdict = "대박입니다!";
    else if (rel.rating >= 6.0f) verdict = "호평을 받았습니다.";
    else if (rel.rating >= 4.0f) verdict = "그럭저럭한 평가입니다.";
    else                         verdict = "혹평을 받았습니다...";

    printf("\n  ================ 게 임 출 시 ================\n");
    printf("   \"%s\" (%s / %s)%s\n", rel.title,
           GenreName((int)rel.genre), ScaleName((int)rel.scale),
           early ? " - 조기 출시" : "");
    printf("   평점 %.1f / 10.0  --  %s\n", rel.rating, verdict);
    printf("   첫 달 판매 %d장 예상\n", rel.monthSales);
    printf("  =============================================\n");
    PauseEnter();
}

static void PlanProject(Company *c)
{
    char title[TITLE_LEN];
    int g, s, cost;

    printf("\n  [ 새 게임 기획 ]\n");
    ReadLine("  >> 게임 제목: ", title, sizeof(title));

    printf("\n  장르를 선택하세요.\n");
    for (g = 0; g < GENRE_COUNT; g++)
        printf("   [%d] %s%s\n", g + 1, GenreName(g),
               (Genre)g == c->trend ? "   <-- 지금 유행!" : "");
    g = ReadInt("  >> 선택: ", 1, GENRE_COUNT) - 1;

    printf("\n  규모를 선택하세요.\n");
    for (s = 0; s < SCALE_COUNT; s++)
        printf("   [%d] %s  (기획비 %d만원 / 필요 진행도 %d)\n",
               s + 1, ScaleName(s), SCALE_COST[s], SCALE_NEED[s]);
    s = ReadInt("  >> 선택: ", 1, SCALE_COUNT) - 1;

    cost = SCALE_COST[s];
    if (c->money < cost) {
        printf("  ! 자본금이 부족합니다. (기획비 %d만원 필요)\n", cost);
        PauseEnter();
        return;
    }
    printf("\n  \"%s\" (%s / %s) - 기획비 %d만원\n",
           title, GenreName(g), ScaleName(s), cost);
    if (ReadInt("  [1] 개발 시작   [0] 취소 : ", 0, 1) == 0)
        return;

    c->money -= cost;
    memset(&c->proj, 0, sizeof(c->proj));
    strcpy(c->proj.title, title);
    c->proj.genre = (Genre)g;
    c->proj.scale = (Scale)s;
    c->proj.need = SCALE_NEED[s];
    c->proj.active = 1;
    printf("  * 개발을 시작했습니다! 다음 달부터 진행됩니다.\n");
    PauseEnter();
}

void DevMenu(Company *c)
{
    Project *p = &c->proj;
    int sel;

    ClearScreen();
    PrintHeader(c);
    printf("  [ 게임 개발 ]\n");

    if (!p->active) {
        printf("\n  진행 중인 프로젝트가 없습니다.\n");
        if (ReadInt("  [1] 새 게임 기획   [0] 뒤로 : ", 0, 1) == 1)
            PlanProject(c);
        return;
    }

    printf("\n  개발중 : \"%s\"  (%s / %s)\n",
           p->title, GenreName((int)p->genre), ScaleName((int)p->scale));
    printf("  진행도 ");
    DrawGauge(p->progress, p->need, 20);
    printf("  %d / %d\n", p->progress, p->need);
    printf("  품질 %d   재미 %d          개발 %d개월차\n",
           p->quality, p->fun, p->devMonths);
    if (p->progress >= p->need)
        printf("  * 개발 완료! 다음 달 진행 시 출시됩니다.\n");
    printf("--------------------------------------------------\n");
    printf("  [1] 계속 개발(뒤로)   [2] 조기 출시   [0] 뒤로\n");
    sel = ReadInt("  >> 선택: ", 0, 2);
    if (sel == 2) {
        printf("  ! 진행도가 모자란 채 출시하면 평점이 깎입니다.\n");
        if (ReadInt("  [1] 그래도 출시   [0] 취소 : ", 0, 1) == 1)
            ReleaseNow(c, 1);
    }
}
