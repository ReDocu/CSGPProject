#pragma once

// ============================================================================
// Full-width console glyphs, expressed as CP949 byte sequences.
//
// This file is PURE ASCII (hex escapes only), so it is safe to open/save in
// any editor without corrupting the multi-byte glyphs. Game sources may keep
// their inline CP949 literals, but new/shared code should use these constants.
//
// Each glyph occupies 2 console columns: draw at (logicalX * 2, y), or via
// ScreenManager::DrawCell(logicalX, y, ...).
//
// Note: each byte is written as its own \xNN escape ("\xA1\xE1") so the
// compiler does not greedily merge them into one oversized escape.
// ============================================================================

#define GLYPH_BLOCK        "\xA1\xE1"  /* filled square   U+25A0  block */
#define GLYPH_BALL         "\xA1\xDC"  /* filled circle   U+25CF        */
#define GLYPH_CURSOR       "\xA2\xBA"  /* right triangle  U+25B6  menu  */
#define GLYPH_STAR         "\xA1\xDA"  /* black star      U+2605        */
#define GLYPH_TRI_DOWN     "\xA1\xE5"  /* down triangle   U+25BC        */
#define GLYPH_TRI_UP       "\xA1\xE3"  /* up triangle     U+25B2        */
#define GLYPH_SQUARE_EMPTY "\xA1\xE0"  /* hollow square   U+25A1        */
#define GLYPH_DIAMOND      "\xA1\xDF"  /* black diamond   U+25C6        */
#define GLYPH_SPADE        "\xA2\xBC"  /* spade           U+2660        */
#define GLYPH_HEART        "\xA2\xBE"  /* heart           U+2665        */
#define GLYPH_OMEGA        "\xA5\xD8"  /* greek omega     U+03A9  ghost */
#define GLYPH_CIRCLE_D     "\xA1\xDD"  /* bullseye        U+25CE  eyes  */
#define GLYPH_MIDDOT       "\xA1\xA4"  /* middle dot      U+00B7  pellet*/
