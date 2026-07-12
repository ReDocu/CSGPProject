#ifndef UI_H
#define UI_H

#include "company.h"

void ClearScreen(void);
void FlushLine(void);
void PauseEnter(void);
int  ReadInt(const char *prompt, int minv, int maxv);
void ReadLine(const char *prompt, char *buf, int size);

const char *GenreName(int g);
const char *ScaleName(int s);

void MoneyStr(int v, char *buf, int size);      /* 4350 -> "4,350" */
void DrawGauge(int val, int maxv, int width);   /* [######----] */
void PrintHeader(const Company *c);

#endif
