#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "company.h"
#include "employee.h"
#include "project.h"
#include "market.h"
#include "savefile.h"
#include "ui.h"

static void TitleScreen(Company *c)
{
    char name[NAME_LEN];
    int sel;

    for (;;) {
        ClearScreen();
        printf("\n");
        printf("  ==================================================\n");
        printf("       G A M E   D E V   T Y C O O N\n");
        printf("           - 게임회사 운영하기 -\n");
        printf("  ==================================================\n");
        printf("       C 기본 문법 스터디 (CSGP 1부)  v1.0\n\n");
        printf("   [1] 새 게임    [2] 불러오기    [0] 종료\n");
        sel = ReadInt("   >> 선택: ", 0, 2);

        if (sel == 0) {
            printf("  안녕히 가세요!\n");
            exit(0);
        }
        if (sel == 2) {
            if (LoadGame(c)) {
                printf("\n  * \"%s\" 세이브를 불러왔습니다. (%d년 %d월)\n",
                       c->name, c->year, c->month);
                PauseEnter();
                return;
            }
            printf("  ! 세이브 파일이 없거나 손상됐습니다.\n");
            PauseEnter();
            continue;
        }
        ReadLine("   >> 회사 이름: ", name, sizeof(name));
        InitCompany(c, name);
        printf("\n  * [%s] 창업! 자본금 %d만원으로 시작합니다.\n", c->name, c->money);
        printf("  * 게임을 개발해 자산 %d만원을 모으거나\n", WIN_MONEY);
        printf("    평점 %.1f 이상의 명작을 만들면 승리합니다.\n", MASTERPIECE_RATING);
        printf("  * 매달 임대료 %d만원이 나갑니다. 자본금이 0 미만이 되면 파산!\n",
               OFFICE_RENT);
        PauseEnter();
        return;
    }
}

/* returns 1 when the player quits */
static int MainMenu(Company *c)
{
    ClearScreen();
    PrintHeader(c);
    printf("  [1] 게임 개발        [2] 직원 관리\n");
    printf("  [3] 회사 현황        [4] 출시작 목록\n");
    printf("  [5] 저장하기         [6] 다음 달 진행 >>\n");
    printf("  [0] 게임 종료\n");
    printf("--------------------------------------------------\n");

    switch (ReadInt("  >> 선택: ", 0, 6)) {
    case 1: DevMenu(c);      break;
    case 2: StaffMenu(c);    break;
    case 3: StatusScreen(c); break;
    case 4: ReleaseList(c);  break;
    case 5:
        if (SaveGame(c))
            printf("  * 저장했습니다. (%s)\n", SAVE_FILE);
        else
            printf("  ! 저장에 실패했습니다.\n");
        PauseEnter();
        break;
    case 6: AdvanceMonth(c); break;
    case 0:
        if (ReadInt("  정말 종료할까요? (저장은 [5]) [1] 예 [0] 아니오 : ", 0, 1) == 1)
            return 1;
        break;
    }
    return 0;
}

static void GameLoop(Company *c)
{
    for (;;) {
        if (MainMenu(c))
            return;                             /* 플레이어가 직접 종료 */
        if (c->ending == END_BANKRUPT)
            return;
        if (c->ending == END_RICH || c->ending == END_MASTERPIECE) {
            ClearScreen();
            printf("\n  **************************************************\n");
            if (c->ending == END_RICH) {
                printf("   축하합니다! 자산 %d만원을 달성했습니다!\n", WIN_MONEY);
                c->achievedRich = 1;
            } else {
                printf("   축하합니다! 평점 %.1f의 명작을 출시했습니다!\n",
                       c->bestRating);
                c->achievedMasterpiece = 1;
            }
            printf("  **************************************************\n");
            printf("   (창업 후 %d개월 만의 성과)\n\n", TotalMonths(c));
            if (ReadInt("  [1] 계속 경영하기   [0] 엔딩 보기 : ", 0, 1) == 1) {
                c->ending = END_NONE;
                continue;
            }
            return;
        }
    }
}

static void EndingScreen(const Company *c)
{
    char m[24];
    int i, totalSold = 0;

    for (i = 0; i < c->shelfCount; i++)
        totalSold += c->shelf[i].totalSales;

    ClearScreen();
    printf("\n  ==================================================\n");
    switch (c->ending) {
    case END_BANKRUPT:
        printf("   파 산 ...  [%s] 은(는) 문을 닫았습니다.\n", c->name);
        break;
    case END_RICH:
        printf("   거상 엔딩!  [%s] 은(는) 업계의 큰손이 됐습니다.\n", c->name);
        break;
    case END_MASTERPIECE:
        printf("   명작 엔딩!  [%s] 의 이름은 게임 역사에 남았습니다.\n", c->name);
        break;
    default:
        printf("   [%s] 의 이야기는 여기서 잠시 멈춥니다.\n", c->name);
        break;
    }
    printf("  ==================================================\n\n");
    MoneyStr(c->money, m, sizeof(m));
    printf("   경영 기간   %d개월 (%d년 %d월까지)\n",
           TotalMonths(c), c->year, c->month);
    printf("   최종 자본   %s만원\n", m);
    printf("   출시작      %d작품 / 누적 판매 %d장\n", c->shelfCount, totalSold);
    printf("   최고 평점   %.1f\n", c->bestRating);
    printf("   명성        %d\n", c->fame);
    printf("\n   플레이해 주셔서 감사합니다!\n\n");
}

int main(int argc, char *argv[])
{
    Company company;

    if (argc > 1)
        srand((unsigned)atoi(argv[1]));     /* 시드 고정 (디버그/테스트용) */
    else
        srand((unsigned)time(NULL));

    TitleScreen(&company);
    GameLoop(&company);
    EndingScreen(&company);
    FreeCompany(&company);
    return 0;
}
