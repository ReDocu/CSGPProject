#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "savefile.h"
#include "employee.h"

#define SAVE_MAGIC "CSGP_TYCOON_SAVE"
#define SAVE_VER   1

/* spaces break fscanf(%s), so store them as '_' (GDD 9) */
static void Encode(char *dst, const char *src, int size)
{
    int i;
    for (i = 0; i < size - 1 && src[i] != '\0'; i++)
        dst[i] = (src[i] == ' ') ? '_' : src[i];
    dst[i] = '\0';
}

static void Decode(char *s)
{
    for (; *s != '\0'; s++)
        if (*s == '_')
            *s = ' ';
}

int SaveGame(const Company *c)
{
    char buf[TITLE_LEN];
    int i;
    FILE *fp = fopen(SAVE_FILE, "w");
    if (fp == NULL)
        return 0;

    fprintf(fp, "%s %d\n", SAVE_MAGIC, SAVE_VER);
    Encode(buf, c->name, NAME_LEN);
    fprintf(fp, "%s\n", buf);
    fprintf(fp, "%d %d %d %d\n", c->money, c->fame, c->year, c->month);

    fprintf(fp, "%d\n", c->staffCount);
    for (i = 0; i < c->staffCount; i++) {
        Encode(buf, c->staff[i].name, NAME_LEN);
        fprintf(fp, "%s %d %d %d %d\n", buf, c->staff[i].dev, c->staff[i].cre,
                c->staff[i].salary, c->staff[i].months);
    }

    fprintf(fp, "%d\n", c->proj.active);
    if (c->proj.active) {
        Encode(buf, c->proj.title, TITLE_LEN);
        fprintf(fp, "%s %d %d %d %d %d %d %d\n", buf,
                (int)c->proj.genre, (int)c->proj.scale,
                c->proj.progress, c->proj.need,
                c->proj.quality, c->proj.fun, c->proj.devMonths);
    }

    fprintf(fp, "%d\n", c->shelfCount);
    for (i = 0; i < c->shelfCount; i++) {
        const Release *r = &c->shelf[i];
        Encode(buf, r->title, TITLE_LEN);
        fprintf(fp, "%s %d %d %.2f %d %d %d\n", buf,
                (int)r->genre, (int)r->scale, r->rating,
                r->monthSales, r->totalSales, r->monthsOnSale);
    }

    fprintf(fp, "%d\n", (int)c->trend);
    fclose(fp);
    return 1;
}

int LoadGame(Company *c)
{
    char magic[32];
    int ver, i, n, g, s, active, trend;
    FILE *fp = fopen(SAVE_FILE, "r");
    if (fp == NULL)
        return 0;

    if (fscanf(fp, "%31s %d", magic, &ver) != 2 ||
        strcmp(magic, SAVE_MAGIC) != 0 || ver != SAVE_VER) {
        fclose(fp);
        return 0;
    }

    InitCompany(c, "");     /* allocates staff array */

    if (fscanf(fp, "%19s", c->name) != 1) goto fail;
    Decode(c->name);
    if (fscanf(fp, "%d %d %d %d", &c->money, &c->fame, &c->year, &c->month) != 4)
        goto fail;

    if (fscanf(fp, "%d", &n) != 1 || n < 0) goto fail;
    for (i = 0; i < n; i++) {
        Employee e;
        if (fscanf(fp, "%19s %d %d %d %d",
                   e.name, &e.dev, &e.cre, &e.salary, &e.months) != 5)
            goto fail;
        Decode(e.name);
        if (c->staffCount == c->staffCap) {
            Employee *tmp = (Employee *)realloc(c->staff,
                                sizeof(Employee) * c->staffCap * 2);
            if (tmp == NULL) goto fail;
            c->staff = tmp;
            c->staffCap *= 2;
        }
        c->staff[c->staffCount++] = e;
    }

    if (fscanf(fp, "%d", &active) != 1) goto fail;
    if (active) {
        Project *p = &c->proj;
        if (fscanf(fp, "%29s %d %d %d %d %d %d %d", p->title, &g, &s,
                   &p->progress, &p->need, &p->quality, &p->fun,
                   &p->devMonths) != 8)
            goto fail;
        Decode(p->title);
        p->genre = (Genre)g;
        p->scale = (Scale)s;
        p->active = 1;
    }

    if (fscanf(fp, "%d", &n) != 1 || n < 0 || n > MAX_SHELF) goto fail;
    for (i = 0; i < n; i++) {
        Release *r = &c->shelf[i];
        if (fscanf(fp, "%29s %d %d %f %d %d %d", r->title, &g, &s,
                   &r->rating, &r->monthSales, &r->totalSales,
                   &r->monthsOnSale) != 7)
            goto fail;
        Decode(r->title);
        r->genre = (Genre)g;
        r->scale = (Scale)s;
    }
    c->shelfCount = n;

    if (fscanf(fp, "%d", &trend) != 1 || trend < 0 || trend >= GENRE_COUNT)
        goto fail;
    c->trend = (Genre)trend;
    fclose(fp);

    /* rebuild runtime-only fields */
    for (i = 0; i < c->shelfCount; i++)
        if (c->shelf[i].rating > c->bestRating)
            c->bestRating = c->shelf[i].rating;
    c->achievedMasterpiece = (c->bestRating >= MASTERPIECE_RATING);
    c->achievedRich = (c->money >= WIN_MONEY);
    RollApplicants(c);
    return 1;

fail:
    fclose(fp);
    FreeCompany(c);
    return 0;
}
