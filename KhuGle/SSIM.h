#pragma once

#include <vector>

inline std::vector<std::vector<unsigned char>> extract_block(unsigned char** image, int x, int y)
{
	std::vector<std::vector<unsigned char>> block(8, std::vector<unsigned char>(8));
	for (int i = 0; i < 8; i++)
		for (int j = 0; j < 8; j++)
			block[i][j] = image[x + i][y + j];
	return block;
}

inline double average(std::vector<std::vector<unsigned char>>& block)
{
	double sum = 0.0;
	for (int i = 0; i < 8; i++)
		for (int j = 0; j < 8; j++)
			sum += block[i][j];
	return sum / 64.0;
}

inline double variance(std::vector<std::vector<unsigned char>>& block, double avg)
{
	double var = 0.0;
	for (int i = 0; i < 8; i++)
		for (int j = 0; j < 8; j++)
			var += (block[i][j] - avg) * (block[i][j] - avg);
	return var / 64.0;
}

inline double covariance(std::vector<std::vector<unsigned char>>& block1,
                         std::vector<std::vector<unsigned char>>& block2, double avg1, double avg2)
{
	double cov = 0.0;
	for (int i = 0; i < 8; i++)
		for (int j = 0; j < 8; j++)
			cov += (block1[i][j] - avg1) * (block2[i][j] - avg2);
	return cov / 64.0;
}

inline double ssim(std::vector<std::vector<unsigned char>>& block1, std::vector<std::vector<unsigned char>>& block2)
{
	double L = 255.0;
	double K1 = 0.01, K2 = 0.03;
	double C1 = (K1 * L) * (K1 * L), C2 = (K2 * L) * (K2 * L);

	double avg1 = average(block1), avg2 = average(block2);
	double var1 = variance(block1, avg1), var2 = variance(block2, avg2);
	double cov = covariance(block1, block2, avg1, avg2);

	double ssim = (2.0 * avg1 * avg2 + C1) * (2.0 * cov + C2) /
		((avg1 * avg1 + avg2 * avg2 + C1) * (var1 + var2 + C2));
	return ssim;
}

inline double compute_ssim(unsigned char** image1, unsigned char** image2, int width, int height)
{
	std::vector<double> ssim_values;
	for (int i = 0; i <= height - 8; i += 8)
		for (int j = 0; j <= width - 8; j += 8)
		{
			auto block1 = extract_block(image1, i, j);
			auto block2 = extract_block(image2, i, j);
			ssim_values.push_back(ssim(block1, block2));
		}

	double sum = 0.0;
	for (double val : ssim_values)
		sum += val;
	return sum / ssim_values.size();
}
