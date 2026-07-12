#ifndef MARKET_H
#define MARKET_H

#include "company.h"

void UpdateSales(Company *c);           /* monthly step 3 */
void MonthlyEvent(Company *c);          /* monthly step 4 */
void UpdateTrend(Company *c, int announce);
void ReleaseList(const Company *c);

#endif
