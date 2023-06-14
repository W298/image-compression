#pragma once

#include <stdlib.h>

double* tempbank = 0;

inline void FWT97(double* x, int n)
{
	double a;
	int i;

	// Predict 1
	a = -1.586134342;
	for (i = 1; i < n - 2; i += 2)
	{
		x[i] += a * (x[i - 1] + x[i + 1]);
	}
	x[n - 1] += 2 * a * x[n - 2];

	// Update 1
	a = -0.05298011854;
	for (i = 2; i < n; i += 2)
	{
		x[i] += a * (x[i - 1] + x[i + 1]);
	}
	x[0] += 2 * a * x[1];

	// Predict 2
	a = 0.8829110762;
	for (i = 1; i < n - 2; i += 2)
	{
		x[i] += a * (x[i - 1] + x[i + 1]);
	}
	x[n - 1] += 2 * a * x[n - 2];

	// Update 2
	a = 0.4435068522;
	for (i = 2; i < n; i += 2)
	{
		x[i] += a * (x[i - 1] + x[i + 1]);
	}
	x[0] += 2 * a * x[1];

	// Scale
	a = 1 / 1.149604398;
	for (i = 0; i < n; i++)
	{
		if (i % 2) x[i] *= a;
		else x[i] /= a;
	}

	// Pack
	if (tempbank == 0) tempbank = (double*)malloc(n * sizeof(double));
	for (i = 0; i < n; i++)
	{
		if (i % 2 == 0) tempbank[i / 2] = x[i];
		else tempbank[n / 2 + i / 2] = x[i];
	}
	for (i = 0; i < n; i++) x[i] = tempbank[i];
}

inline void IWT97(double* x, int n)
{
	double a;
	int i;

	// Unpack
	if (tempbank == 0) tempbank = (double*)malloc(n * sizeof(double));
	for (i = 0; i < n / 2; i++)
	{
		tempbank[i * 2] = x[i];
		tempbank[i * 2 + 1] = x[i + n / 2];
	}
	for (i = 0; i < n; i++) x[i] = tempbank[i];

	// Undo scale
	a = 1.149604398;
	for (i = 0; i < n; i++)
	{
		if (i % 2) x[i] *= a;
		else x[i] /= a;
	}

	// Undo update 2
	a = -0.4435068522;
	for (i = 2; i < n; i += 2)
	{
		x[i] += a * (x[i - 1] + x[i + 1]);
	}
	x[0] += 2 * a * x[1];

	// Undo predict 2
	a = -0.8829110762;
	for (i = 1; i < n - 2; i += 2)
	{
		x[i] += a * (x[i - 1] + x[i + 1]);
	}
	x[n - 1] += 2 * a * x[n - 2];

	// Undo update 1
	a = 0.05298011854;
	for (i = 2; i < n; i += 2)
	{
		x[i] += a * (x[i - 1] + x[i + 1]);
	}
	x[0] += 2 * a * x[1];

	// Undo predict 1
	a = 1.586134342;
	for (i = 1; i < n - 2; i += 2)
	{
		x[i] += a * (x[i - 1] + x[i + 1]);
	}
	x[n - 1] += 2 * a * x[n - 2];

	tempbank = 0;
}

inline void FWT2D(double** img, int n)
{
	// row-wise transform
	for (int i = 0; i < n; i++)
	{
		FWT97(img[i], n);
	}
	// column-wise transform
	for (int i = 0; i < n; i++)
	{
		double* col = new double[n];
		for (int j = 0; j < n; j++)
		{
			col[j] = img[j][i];
		}
		FWT97(col, n);
		for (int j = 0; j < n; j++)
		{
			img[j][i] = col[j];
		}
		delete[] col;
	}
}

void IWT2D(double** img, int n)
{
	// column-wise transform
	for (int i = 0; i < n; i++)
	{
		double* col = new double[n];
		for (int j = 0; j < n; j++)
		{
			col[j] = img[j][i];
		}
		IWT97(col, n);
		for (int j = 0; j < n; j++)
		{
			img[j][i] = col[j];
		}
		delete[] col;
	}
	// row-wise transform
	for (int i = 0; i < n; i++)
	{
		IWT97(img[i], n);
	}
}
