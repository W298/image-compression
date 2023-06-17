#pragma once

#include "KhuGleBase.h"

inline unsigned char Bilinear(unsigned char** src, float x, float y, int srcHeight, int srcWidth)
{
	int x1 = floor(x);
	int y1 = floor(y);
	int x2 = min(x1 + 1, srcWidth - 1);
	int y2 = min(y1 + 1, srcHeight - 1);

	unsigned char Q11 = src[y1][x1];
	unsigned char Q21 = src[y1][x2];
	unsigned char Q12 = src[y2][x1];
	unsigned char Q22 = src[y2][x2];

	float R1 = ((x2 - x) / (x2 - x1)) * Q11 + ((x - x1) / (x2 - x1)) * Q21;
	float R2 = ((x2 - x) / (x2 - x1)) * Q12 + ((x - x1) / (x2 - x1)) * Q22;

	unsigned char P = ((y2 - y) / (y2 - y1)) * R1 + ((y - y1) / (y2 - y1)) * R2;

	return P;
}

inline unsigned char** DownSampleImage(unsigned char** src, int srcHeight, int srcWidth, int dstHeight, int dstWidth)
{
	unsigned char** dst = cmatrix(dstHeight, dstWidth);

	const float scaleX = (float)srcWidth / (float)dstWidth;
	const float scaleY = (float)srcHeight / (float)dstHeight;

	for (int y = 0; y < dstHeight; ++y)
		for (int x = 0; x < dstWidth; ++x)
			dst[y][x] = Bilinear(src, x * scaleX, y * scaleY, srcHeight, srcWidth);

	return dst;
}
