#include "RenderUtil.h"
#include "Glyph.h"

void RenderUtil::DrawSpriteRows(int leftX, int topY, const char* const rows[], int h, int color)
{
	for (int r = 0; r < h; r++)
	{
		const char* row = rows[r];
		for (int c = 0; row[c] != '\0'; c++)
		{
			if (row[c] != ' ')
				SCREEN->OnDrawColor((leftX + c) * 2, topY + r, GLYPH_BLOCK, (unsigned short)color);
		}
	}
}

void RenderUtil::DrawGauge(short x, short y, int val, int maxv, int width)
{
	char buf[64];
	if (width > 60) width = 60;
	int fill = (maxv > 0) ? val * width / maxv : 0;
	if (fill > width) fill = width;
	if (fill < 0) fill = 0;

	int o = 0;
	buf[o++] = '[';
	for (int i = 0; i < width; i++)
		buf[o++] = (i < fill) ? '#' : '-';
	buf[o++] = ']';
	buf[o] = '\0';
	SCREEN->OnDrawColor(x, y, buf, GREEN);
}

void RenderUtil::DrawBorder(unsigned short color)
{
	for (short y = 0; y < GAME_SIZE_Y; y++)
		for (short x = 0; x < GAME_SIZE_X; x++)
			if (x == 0 || y == 0 || x == GAME_SIZE_X - 1 || y == GAME_SIZE_Y - 1)
				SCREEN->OnDrawColor(x * 2, y, GLYPH_BLOCK, color);
}
