#pragma once
#include "../../framework.h"

// ============================================================================
// Shared rendering helpers. Pure ASCII - edit-safe.
//
// Game logic works in logical coordinates (40x25); these helpers apply the
// "x * 2" full-width conversion internally where noted.
// ============================================================================
namespace RenderUtil
{
	// Draw a multi-row sprite. Each row is a string; ' ' = transparent, any
	// other char = one filled block (GLYPH_BLOCK). leftX/topY are LOGICAL.
	void DrawSpriteRows(int leftX, int topY, const char* const rows[], int h, int color);

	// Horizontal fill gauge "[####----]" at CONSOLE coords x,y.
	void DrawGauge(short x, short y, int val, int maxv, int width);

	// Outer frame around the 40x25 play field (drawn in full-width blocks).
	void DrawBorder(unsigned short color = GRAY);
}
