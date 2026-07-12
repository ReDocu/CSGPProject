#ifndef COMPANY_H
#define COMPANY_H

/* ===== balance constants (ch15: tune everything here) ===== */
#define START_YEAR          2026
#define START_MONEY         5000    /* unit: 10,000 KRW */
#define OFFICE_RENT         100
#define WIN_MONEY           10000
#define MASTERPIECE_RATING  9.0f
#define BOSS_DEV            3       /* the player also works */
#define BOSS_CRE            3
#define SALARY_RATE         30      /* salary = (dev+cre) * 30 */
#define BASE_PROGRESS       15      /* monthly progress = 15 + sum(dev) */
#define START_STAFF_CAP     4
#define MAX_APPLICANTS      3
#define MAX_SHELF           50
#define MAX_SALE_MONTHS     6
#define MIN_SALES           100     /* below this, a title goes off sale */
#define EVENT_CHANCE        30      /* % per month */
#define RAISE_MONTHS        6       /* service months before raise demand */

#define NAME_LEN            20
#define TITLE_LEN           30

typedef enum { G_RPG, G_ACTION, G_PUZZLE, G_SIM, G_SPORTS, GENRE_COUNT } Genre;
typedef enum { S_SMALL, S_MEDIUM, S_LARGE, SCALE_COUNT } Scale;
typedef enum { END_NONE, END_BANKRUPT, END_RICH, END_MASTERPIECE } Ending;

typedef struct {
    char name[NAME_LEN];
    int  dev, cre;              /* 1~10 */
    int  salary;                /* per month */
    int  months;                /* months served */
} Employee;

typedef struct {
    char  title[TITLE_LEN];
    Genre genre;
    Scale scale;
    int   progress, need;
    int   quality, fun;
    int   devMonths;
    int   active;               /* 0 = no project */
} Project;

typedef struct {
    char  title[TITLE_LEN];
    Genre genre;
    Scale scale;
    float rating;               /* 0.0 ~ 10.0 */
    int   monthSales;           /* sales this month, 0 = off sale */
    int   totalSales;
    int   monthsOnSale;
} Release;

typedef struct {
    char      name[NAME_LEN];
    int       money, fame;
    int       year, month;
    Employee *staff;            /* dynamic array (ch11) */
    int       staffCount, staffCap;
    Project   proj;
    Release   shelf[MAX_SHELF];
    int       shelfCount;
    Genre     trend;
    /* runtime only (not saved, rebuilt on load) */
    Employee  applicants[MAX_APPLICANTS];
    int       applicantCount;
    Ending    ending;
    int       achievedRich, achievedMasterpiece;
    float     bestRating;
} Company;

void InitCompany(Company *c, const char *name);
void FreeCompany(Company *c);
void AdvanceMonth(Company *c);
void StatusScreen(const Company *c);
int  TotalMonths(const Company *c);

/* monthly report lines (buffer owned by company.c) */
void ResetReport(void);
void Report(const char *label, int amount);

#endif
