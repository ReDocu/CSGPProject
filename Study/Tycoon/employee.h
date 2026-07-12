#ifndef EMPLOYEE_H
#define EMPLOYEE_H

#include "company.h"

void StaffMenu(Company *c);
void RollApplicants(Company *c);        /* new applicants each month */
void AddScoutApplicant(Company *c);     /* event: high-stat applicant */
int  TotalSalary(const Company *c);
int  DevSum(const Company *c);          /* staff + boss */
int  CreSum(const Company *c);
void AgeStaff(Company *c);
void QuitEmployee(Company *c, int idx); /* leaves, no severance */

#endif
