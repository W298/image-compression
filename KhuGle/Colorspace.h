#pragma once

#include "KhuGleWin.h"

inline void RGB2YCbCr(double** R, double** G, double** B, double** Y, double** Cb, double** Cr, int height, int width)
{
	for (int y = 0; y < height; y++)
	{
		for (int x = 0; x < width; x++)
		{
			Y[y][x] = R[y][x] * 0.299 + G[y][x] * 0.587 + B[y][x] * 0.114;
		}
	}

	for (int y = 0; y < height; y += 2)
	{
		for (int x = 0; x < width; x += 2)
		{
			Cb[y / 2][x / 2] = R[y][x] * -0.16874 + G[y][x] * -0.33126 + B[y][x] * 0.5;
			Cr[y / 2][x / 2] = R[y][x] * 0.5 + G[y][x] * -0.41869 + B[y][x] * -0.08131;
		}
	}
}

inline void YCbCr2RGB(double** Y, double** Cb, double** Cr, double** R, double** G, double** B, int height, int width)
{
	for (int y = 0; y < height; y++)
	{
		for (int x = 0; x < width; x++)
		{
			R[y][x] = max(0, min(Y[y][x] * 1 + Cr[y / 2][x / 2] * 1.402, 255));
			G[y][x] = max(0, min(Y[y][x] * 1 + Cb[y / 2][x / 2] * -0.34414 + Cr[y / 2][x / 2] * -0.71414, 255));
			B[y][x] = max(0, min(Y[y][x] * 1 + Cb[y / 2][x / 2] * 1.772, 255));
		}
	}
}
