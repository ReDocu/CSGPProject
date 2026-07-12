#ifndef SAVEFILE_H
#define SAVEFILE_H

#include "company.h"

#define SAVE_FILE "save.txt"

int SaveGame(const Company *c);   /* returns 1 on success */
int LoadGame(Company *c);         /* returns 1 on success */

#endif
