#pragma once
#include <string>

// ============================================================================
// Small text helpers shared across scenes. Pure ASCII - edit-safe.
// ============================================================================
namespace TextUtil
{
	// Zero-pad a number to a fixed width:  Pad(42, 6) -> "000042"
	std::string Pad(int v, int width);

	// Thousands separator:  Comma(1234567) -> "1,234,567"  (handles negatives)
	std::string Comma(int v);

	// Center x for an 80-column screen given the text's byte length:
	//   40 - bytes / 2  (full-width chars count as 2 bytes)
	short Center(int textBytes);
}
