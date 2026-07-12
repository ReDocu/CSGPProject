#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ui.h"

void ClearScreen(void)
{
    system("cls");
}

/* 현재 줄 끝까지 입력 버퍼 비우기 - scanf 뒤에 남는 개행 처리 (8장) */
void FlushLine(void)
{
    int ch;
    while ((ch = getchar()) != '\n') {
        if (ch == EOF) {
            printf("\n입력이 끊겨 게임을 종료합니다.\n");
            exit(1);
        }
    }
}

void PauseEnter(void)
{
    printf("\n  (Enter를 누르면 계속)");
    FlushLine();
}

int ReadInt(const char *prompt, int minv, int maxv)
{
    int v, r;
    for (;;) {
        printf("%s", prompt);
        r = scanf("%d", &v);
        if (r == EOF) {
            printf("\n입력이 끊겨 게임을 종료합니다.\n");
            exit(1);
        }
        FlushLine();
        if (r == 1 && v >= minv && v <= maxv)
            return v;
        printf("  ! %d~%d 사이의 숫자를 입력하세요.\n", minv, maxv);
    }
}

void ReadLine(const char *prompt, char *buf, int size)
{
    size_t len;
    for (;;) {
        printf("%s", prompt);
        if (fgets(buf, size, stdin) == NULL) {
            printf("\n입력이 끊겨 게임을 종료합니다.\n");
            exit(1);
        }
        len = strlen(buf);
        if (len > 0 && buf[len - 1] != '\n')
            FlushLine();                    /* 너무 긴 입력의 나머지 버리기 */
        buf[strcspn(buf, "\n")] = '\0';
        if (buf[0] != '\0')
            return;
    }
}

const char *GenreName(int g)
{
    static const char *NAMES[GENRE_COUNT] = {
        "RPG", "액션", "퍼즐", "시뮬레이션", "스포츠"
    };
    return (g >= 0 && g < GENRE_COUNT) ? NAMES[g] : "?";
}

const char *ScaleName(int s)
{
    static const char *NAMES[SCALE_COUNT] = { "소형", "중형", "대형" };
    return (s >= 0 && s < SCALE_COUNT) ? NAMES[s] : "?";
}

/* 3자리 콤마 표기 - printf 서식 실습의 연장 (2장) */
void MoneyStr(int v, char *buf, int size)
{
    char digits[16];
    char out[24];
    int  n, i, o = 0, neg = 0;

    if (v < 0) { neg = 1; v = -v; }
    sprintf(digits, "%d", v);
    n = (int)strlen(digits);
    for (i = 0; i < n; i++) {
        if (i > 0 && (n - i) % 3 == 0)
            out[o++] = ',';
        out[o++] = digits[i];
    }
    out[o] = '\0';
    snprintf(buf, size, "%s%s", neg ? "-" : "", out);
}

void DrawGauge(int val, int maxv, int width)
{
    int fill = (maxv > 0) ? val * width / maxv : 0;
    int i;
    if (fill > width) fill = width;
    if (fill < 0) fill = 0;
    printf("[");
    for (i = 0; i < width; i++)
        putchar(i < fill ? '#' : '-');
    printf("]");
}

void PrintHeader(const Company *c)
{
    char m[24];
    MoneyStr(c->money, m, sizeof(m));
    printf("==================================================\n");
    printf("  %-16s   %d년 %2d월     명성 ★ %d\n",
           c->name, c->year, c->month, c->fame);
    printf("  자본금 %s만원 | 직원 %d명 | 개발중 %d건 | 유행 [%s]\n",
           m, c->staffCount, c->proj.active, GenreName((int)c->trend));
    printf("==================================================\n");
}
