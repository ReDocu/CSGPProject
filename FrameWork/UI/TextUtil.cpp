#include "TextUtil.h"
#include <cstdio>
#include <cstring>

std::string TextUtil::Pad(int v, int width)
{
	std::string s = std::to_string(v);
	while ((int)s.size() < width)
		s = "0" + s;
	return s;
}

std::string TextUtil::Comma(int v)
{
	char digits[16];
	char out[24];
	int  neg = 0;
	if (v < 0) { neg = 1; v = -v; }
	snprintf(digits, sizeof(digits), "%d", v);
	int n = (int)strlen(digits);
	int o = 0;
	for (int i = 0; i < n; i++)
	{
		if (i > 0 && (n - i) % 3 == 0)
			out[o++] = ',';
		out[o++] = digits[i];
	}
	out[o] = '\0';
	return std::string(neg ? "-" : "") + out;
}

short TextUtil::Center(int textBytes)
{
	short x = (short)(40 - textBytes / 2);
	return x < 0 ? 0 : x;
}
