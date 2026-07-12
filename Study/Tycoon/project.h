#ifndef PROJECT_H
#define PROJECT_H

#include "company.h"

extern const int SCALE_COST[SCALE_COUNT];
extern const int SCALE_NEED[SCALE_COUNT];
extern const int SCALE_SALE_MULT[SCALE_COUNT];
extern const int SCALE_PROFIT_MULT[SCALE_COUNT];

void DevMenu(Company *c);
void ProgressProject(Company *c);       /* monthly step 1 */
int  ProjectDone(const Company *c);
void ReleaseNow(Company *c, int early); /* rating + first-month sales */

#endif
