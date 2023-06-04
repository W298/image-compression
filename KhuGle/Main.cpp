//
//	Dept. Software Convergence, Kyung Hee University
//	Prof. Daeho Lee, nize@khu.ac.kr
//
#include "KhuGleWin.h"
#include "KhuGleSignal.h"
#include "DWT.h"
#include "SSIM.h"
#include "Huffman.h"
#include "Export.h"
#include "Decode.h"
#include <iostream>

#pragma warning(disable:4996)

#define _CRTDBG_MAP_ALLOC
#include <cstdlib>
#include <crtdbg.h>
#include <map>

#ifdef _DEBUG
#ifndef DBG_NEW
#define DBG_NEW new ( _NORMAL_BLOCK , __FILE__ , __LINE__ )
#define new DBG_NEW
#endif
#endif  // _DEBUG

template <typename T> int sgn(T val) {
	return (T(0) < val) - (val < T(0));
}

std::vector<int> flatten(const std::vector<std::vector<int>>& origin, int h_start, int h_end, int w_start, int w_end)
{
	std::vector<int> merged_vector;
	for (int i = h_start; i < h_end; i++)
	{
		for (int j = w_start; j < w_end; j++)
		{
			merged_vector.push_back(origin[i][j]);
		}
	}
	return merged_vector;
}

std::map<double, int> compute_precise_histogram(const std::vector<double>& dwt_coefficients)
{
	std::map<double, int> precise_histogram;
	for (auto coeff : dwt_coefficients)
	{
		double symbol = std::round(coeff * 1000000) / 1000000;
		precise_histogram[symbol]++;
	}
	return precise_histogram;
}

std::map<double, double> compute_precise_quantization_table(const std::map<double, int>& precise_histogram, double base_step_size) {
	std::map<double, double> q_table;

	for (const std::pair<double, int> value : precise_histogram)
	{
		double symbol = value.first;
		int count = value.second;
		q_table[symbol] = base_step_size / std::sqrt(count);
	}

	return q_table;
}

std::map<int, int> compute_histogram(const std::vector<double>& dwt_coefficients) {
	std::map<int, int> histogram;

	for (auto coeff : dwt_coefficients) {
		int bin = std::round(coeff);
		histogram[bin]++;
	}

	return histogram;
}

std::map<int, int> compute_histogram(const std::vector<int>& quantized_values) {
	std::map<int, int> histogram;

	for (auto q : quantized_values) {
		histogram[q]++;
	}

	return histogram;
}

std::map<int, double> compute_quantization_table(const std::map<int, int>& histogram, double base_step_size) {
	std::map<int, double> q_table;

	for (const std::pair<int, int> value : histogram)
	{
		int bin = value.first;
		int count = value.second;
		q_table[bin] = base_step_size / std::sqrt(count);
	}

	return q_table;
}

void print_histogram(const std::map<int, int>& map, const int step = 5)
{
	int count = 0;
	int start = -100;
	int sum = 0;
	int pre = 0;
	for (std::pair<const int, int> pair : map)
	{
		if (count == step)
		{
			std::cout << start << "~" << pre << "\t";
			printf("\x1b[37;47m");
			for (int j = 0; j < max(1, min(sum / 30, 80)); j++)
			{
				std::cout << "#";
			}
			printf("\x1b[39;49m");
			std::cout << "  [" << sum << "]\n";
			count = 0;
			start = -100;
			sum = 0;
		}

		if (start == -100)
		{
			start = pair.first;
		}

		sum += pair.second;
		pre = pair.first;
		count++;
	}
}

std::vector<double> flatten(double** input, int h_start, int h_end, int w_start, int w_end)
{
	std::vector<double> flatted_vector;
	for (int y = h_start; y < h_end; y++)
	{
		for (int x = w_start; x < w_end; x++)
		{
			flatted_vector.push_back(input[y][x]);
		}
	}

	return flatted_vector;
}

class CKhuGleImageLayer : public CKhuGleLayer {
public:
	CKhuGleSignal m_Image, m_ImageOut;

	CKhuGleImageLayer(int nW, int nH, KgColor24 bgColor, CKgPoint ptPos = CKgPoint(0, 0))
		: CKhuGleLayer(nW, nH, bgColor, ptPos) {}
	void DrawBackgroundImage();
};

void CKhuGleImageLayer::DrawBackgroundImage()
{
	for(int y = 0 ; y < m_nH ; y++)
		for(int x = 0 ; x < m_nW ; x++)
		{
			m_ImageBgR[y][x] = KgGetRed(m_bgColor);
			m_ImageBgG[y][x] = KgGetGreen(m_bgColor);
			m_ImageBgB[y][x] = KgGetBlue(m_bgColor);
		}

	if(m_Image.m_Red && m_Image.m_Green && m_Image.m_Blue)
	{
		for(int y = 0 ; y < m_Image.m_nH && y < m_nH ; ++y)
			for(int x = 0 ; x < m_Image.m_nW && x < m_nW ; ++x)
			{
				m_ImageBgR[y][x] = m_Image.m_Red[y][x];
				m_ImageBgG[y][x] = m_Image.m_Green[y][x];
				m_ImageBgB[y][x] = m_Image.m_Blue[y][x];
			}
	}

	if(m_ImageOut.m_Red && m_ImageOut.m_Green && m_ImageOut.m_Blue)
	{
		int OffsetX = 300, OffsetY = 0;
		for(int y = 0 ; y < m_ImageOut.m_nH && y + OffsetY < m_nH ; ++y)
			for(int x = 0 ; x < m_ImageOut.m_nW && x + OffsetX < m_nW ; ++x)
			{
				m_ImageBgR[y + OffsetY][x + OffsetX] = m_ImageOut.m_Red[y][x];
				m_ImageBgG[y + OffsetY][x + OffsetX] = m_ImageOut.m_Green[y][x];
				m_ImageBgB[y + OffsetY][x + OffsetX] = m_ImageOut.m_Blue[y][x];
			}
	}
}

class CImageProcessing : public CKhuGleWin {
public:
	CKhuGleImageLayer *m_pImageLayer;

	CImageProcessing(int nW, int nH, char *ImagePath);
	void Update();
};

CImageProcessing::CImageProcessing(int nW, int nH, char *ImagePath) 
	: CKhuGleWin(nW, nH) {
	m_pScene = new CKhuGleScene(640, 480, KG_COLOR_24_RGB(100, 100, 150));

	m_pImageLayer = new CKhuGleImageLayer(600, 420, KG_COLOR_24_RGB(150, 150, 200), CKgPoint(20, 30));
	m_pImageLayer->m_Image.ReadBmp(ImagePath);
	m_pImageLayer->m_ImageOut.ReadBmp(ImagePath);
	m_pImageLayer->DrawBackgroundImage();
	m_pScene->AddChild(m_pImageLayer);
}

void CImageProcessing::Update()
{
	if(m_bKeyPressed['Y'])
	{
		double **InputR = dmatrix(m_pImageLayer->m_Image.m_nH, m_pImageLayer->m_Image.m_nW);
		double **InputG = dmatrix(m_pImageLayer->m_Image.m_nH, m_pImageLayer->m_Image.m_nW);
		double **InputB = dmatrix(m_pImageLayer->m_Image.m_nH, m_pImageLayer->m_Image.m_nW);

		double **OutR = dmatrix(m_pImageLayer->m_Image.m_nH, m_pImageLayer->m_Image.m_nW);
		double **OutG = dmatrix(m_pImageLayer->m_Image.m_nH, m_pImageLayer->m_Image.m_nW);
		double **OutB = dmatrix(m_pImageLayer->m_Image.m_nH, m_pImageLayer->m_Image.m_nW);

		for(int y = 0 ; y < m_pImageLayer->m_Image.m_nH ; ++y)
			for(int x = 0 ; x < m_pImageLayer->m_Image.m_nW ; ++x)
			{
				InputR[y][x] = m_pImageLayer->m_Image.m_Red[y][x];
				InputG[y][x] = m_pImageLayer->m_Image.m_Green[y][x];
				InputB[y][x] = m_pImageLayer->m_Image.m_Blue[y][x];
			}

		/*
		 *	YCbCr Define & Convert
		 */

		double** InputY = dmatrix(m_pImageLayer->m_Image.m_nH, m_pImageLayer->m_Image.m_nW);
		double** InputCb = dmatrix(m_pImageLayer->m_Image.m_nH / 2, m_pImageLayer->m_Image.m_nW / 2);
		double** InputCr = dmatrix(m_pImageLayer->m_Image.m_nH / 2, m_pImageLayer->m_Image.m_nW / 2);

		double** OutY = dmatrix(m_pImageLayer->m_Image.m_nH, m_pImageLayer->m_Image.m_nW);
		double** OutCb = dmatrix(m_pImageLayer->m_Image.m_nH / 2, m_pImageLayer->m_Image.m_nW / 2);
		double** OutCr = dmatrix(m_pImageLayer->m_Image.m_nH / 2, m_pImageLayer->m_Image.m_nW / 2);

		for (int y = 0; y < m_pImageLayer->m_Image.m_nH; y++)
		{
			for (int x = 0; x < m_pImageLayer->m_Image.m_nW; x++)
			{
				InputY[y][x] =
					m_pImageLayer->m_Image.m_Red[y][x] * 0.299 +
					m_pImageLayer->m_Image.m_Green[y][x] * 0.587 +
					m_pImageLayer->m_Image.m_Blue[y][x] * 0.114;
			}
		}

		for (int y = 0; y < m_pImageLayer->m_Image.m_nH; y += 2)
		{
			for (int x = 0; x < m_pImageLayer->m_Image.m_nW; x += 2)
			{
				InputCb[y/2][x/2] =
					m_pImageLayer->m_Image.m_Red[y][x] * -0.16874 +
					m_pImageLayer->m_Image.m_Green[y][x] * -0.33126 +
					m_pImageLayer->m_Image.m_Blue[y][x] * 0.5;

				InputCr[y/2][x/2] =
					m_pImageLayer->m_Image.m_Red[y][x] * 0.5 +
					m_pImageLayer->m_Image.m_Green[y][x] * -0.41869 +
					m_pImageLayer->m_Image.m_Blue[y][x] * -0.08131;
			}
		}

		/*
		 *	Processing
		 */

		constexpr bool use_dynamic_table = false;

		fwt2D(InputY, m_pImageLayer->m_Image.m_nH);
		fwt2D(InputCb, m_pImageLayer->m_Image.m_nH / 2);
		fwt2D(InputCr, m_pImageLayer->m_Image.m_nH / 2);

		fwt2D(InputY, m_pImageLayer->m_Image.m_nH / 2);
		fwt2D(InputCb, m_pImageLayer->m_Image.m_nH / 4);
		fwt2D(InputCr, m_pImageLayer->m_Image.m_nH / 4);

		int h = m_pImageLayer->m_Image.m_nH;
		int w = m_pImageLayer->m_Image.m_nW;
		int h_half = m_pImageLayer->m_Image.m_nH / 2;
		int w_half = m_pImageLayer->m_Image.m_nW / 2;
		int h_qut = m_pImageLayer->m_Image.m_nH / 4;
		int w_qut = m_pImageLayer->m_Image.m_nW / 4;

		std::vector<std::string> map_name = { "LL1", "LH1", "HL1", "HH1", "LH", "HL", "HH" };
		std::vector<std::pair<std::pair<int, int>, std::pair<int, int>>> map;
		std::vector<double> factor = { 2, 8, 8, 32, 32, 128, 512 };

		map.push_back({ {0, h_qut}, {0, w_qut} });
		map.push_back({ {0, h_qut}, {w_qut, w_half} });
		map.push_back({ {h_qut, h_half}, {0, w_qut} });
		map.push_back({ {h_qut, h_half}, {w_qut, w_half} });
		map.push_back({ {0, h_half}, {w_half, w} });
		map.push_back({ {h_half, h}, {0, w_half} });
		map.push_back({ {h_half, h}, {w_half, w} });

		std::vector<std::vector<int>> quantized_y(m_pImageLayer->m_Image.m_nH, std::vector<int>(m_pImageLayer->m_Image.m_nW));
		std::vector<std::vector<int>> quantized_cb(m_pImageLayer->m_Image.m_nH / 2, std::vector<int>(m_pImageLayer->m_Image.m_nW / 2));
		std::vector<std::vector<int>> quantized_cr(m_pImageLayer->m_Image.m_nH / 2, std::vector<int>(m_pImageLayer->m_Image.m_nW / 2));

		if (!use_dynamic_table)
		{
			int index = 0;
			for (auto sub_band : map)
			{
				double r = 8;
				double i = 2;
				double c = 8;
				double f = 23;

				double tau = pow(2, r - c + i) * (1 + f / pow(2, 11));

				double step_size;
				if (index == 0)
				{
					step_size = tau / pow(2, i);
				}
				else if (index == 1 || index == 2)
				{
					step_size = tau / pow(2, 2 - 1);
				}
				else if (index == 3)
				{
					step_size = tau / pow(2, 2 - 2);
				}
				else if (index == 4 || index == 5)
				{
					step_size = tau / pow(2, 1 - 1);
				}
				else if (index == 6)
				{
					step_size = tau / pow(2, 1 - 2);
				}

				std::cout << step_size << "\t";

				auto h_range = sub_band.first;
				auto w_range = sub_band.second;

				auto flatted_vector_y = flatten(InputY, h_range.first, h_range.second, w_range.first, w_range.second);
				auto his_y = compute_precise_histogram(flatted_vector_y);
				std::cout << map_name[index] << " before symbol count\t" << his_y.size() << "\n";

				for (int y = h_range.first; y < h_range.second; y++)
				{
					for (int x = w_range.first; x < w_range.second; x++)
					{
						int q = (int)std::round(InputY[y][x] / step_size);
						quantized_y[y][x] = q;
						// InputY[y][x] = q * step_size;
					}
				}

				for (int y = h_range.first / 2; y < h_range.second / 2; y++)
				{
					for (int x = w_range.first / 2; x < w_range.second / 2; x++)
					{
						int q = (int)std::round(InputCb[y][x] / step_size);
						quantized_cb[y][x] = q;
						// InputCb[y][x] = q * step_size;

						q = (int)std::round(InputCr[y][x] / step_size);
						quantized_cr[y][x] = q;
						// InputCr[y][x] = q * step_size;
					}
				}

				index++;
			}

			for (int i = 0; i < map.size(); i++)
			{
				auto histogram = compute_precise_histogram(flatten(InputY, map[i].first.first, map[i].first.second, map[i].second.first, map[i].second.second));
				std::cout << map_name[i] << " after symbol count: " << histogram.size() << "\n";
			}
		}
		else
		{
			int symbol_count = 0;
			int index = 0;
			for (auto sub_band : map)
			{
				auto h_range = sub_band.first;
				auto w_range = sub_band.second;

				auto flatted_vector_y = flatten(InputY, h_range.first, h_range.second, w_range.first, w_range.second);
				auto flatted_vector_cb = flatten(InputCb, h_range.first / 2, h_range.second / 2, w_range.first / 2, w_range.second / 2);
				auto flatted_vector_cr = flatten(InputCr, h_range.first / 2, h_range.second / 2, w_range.first / 2, w_range.second / 2);

				auto his_y = compute_precise_histogram(flatted_vector_y);
				std::cout << map_name[index] << " before symbol count\t" << his_y.size() << "\n";
				symbol_count += his_y.size();

				auto qt_y = compute_quantization_table(compute_histogram(flatted_vector_y), 1 * factor[index]);
				auto qt_cb = compute_quantization_table(compute_histogram(flatted_vector_cb), 10 * factor[index]);
				auto qt_cr = compute_quantization_table(compute_histogram(flatted_vector_cr), 10 * factor[index]);

				for (int y = h_range.first; y < h_range.second; y++)
				{
					for (int x = w_range.first; x < w_range.second; x++)
					{
						int symbol = std::round(InputY[y][x]);
						int q = (int)std::round(InputY[y][x] / qt_y[symbol]);
						quantized_y[y][x] = q;
					}
				}

				for (int y = h_range.first / 2; y < h_range.second / 2; y++)
				{
					for (int x = w_range.first / 2; x < w_range.second / 2; x++)
					{
						int symbol_cb = std::round(InputCb[y][x]);
						int q_cb = (int)std::round(InputCb[y][x] / qt_cb[symbol_cb]);
						quantized_cb[y][x] = q_cb;

						int symbol_cr = std::round(InputCr[y][x]);
						int q_cr = (int)std::round(InputCr[y][x] / qt_cr[symbol_cr]);
						quantized_cr[y][x] = q_cr;
					}
				}

				index++;
			}

			std::cout << "total: " << symbol_count << "\n";
			std::cout << "\n\n\n\n";

			int after_symbol_count = 0;
			for (int i = 0; i < map.size(); i++)
			{
				auto histogram = compute_precise_histogram(flatten(InputY, map[i].first.first, map[i].first.second, map[i].second.first, map[i].second.second));
				std::cout << map_name[i] << " after symbol count: " << histogram.size() << "\n";
				after_symbol_count += histogram.size();
			}
			std::cout << "total: " << after_symbol_count << "\n";

			std::cout << "\n\n";
			std::cout << (double)(symbol_count - after_symbol_count) / (double)symbol_count * 100.0 << " % better" << "\n";

			for (int i = 0; i < 16; i++)
			{
				for (int j = 0; j < 16; j++)
				{
					std::cout << quantized_y[i][j] << "\t";
				}
				std::cout << "\n";
			}

			index = 0;
			for (auto sub_band : map)
			{
				auto h_range = sub_band.first;
				auto w_range = sub_band.second;

				auto flatted_vector_y = flatten(quantized_y, h_range.first, h_range.second, w_range.first, w_range.second);
				auto flatted_vector_cb = flatten(quantized_cb, h_range.first / 2, h_range.second / 2, w_range.first / 2, w_range.second / 2);
				auto flatted_vector_cr = flatten(quantized_cr, h_range.first / 2, h_range.second / 2, w_range.first / 2, w_range.second / 2);

				auto qt_y = compute_quantization_table(compute_histogram(flatted_vector_y), 1 * factor[index]);
				auto qt_cb = compute_quantization_table(compute_histogram(flatted_vector_cb), 10 * factor[index]);
				auto qt_cr = compute_quantization_table(compute_histogram(flatted_vector_cr), 10 * factor[index]);

				for (int y = h_range.first; y < h_range.second; y++)
				{
					for (int x = w_range.first; x < w_range.second; x++)
					{
						InputY[y][x] = qt_y[quantized_y[y][x]] * quantized_y[y][x];
					}
				}

				for (int y = h_range.first / 2; y < h_range.second / 2; y++)
				{
					for (int x = w_range.first / 2; x < w_range.second / 2; x++)
					{
						InputCb[y][x] = qt_cb[quantized_cb[y][x]] * quantized_cb[y][x];
						InputCr[y][x] = qt_cr[quantized_cr[y][x]] * quantized_cr[y][x];
					}
				}

				index++;
			}
		}

		std::vector<std::pair<int, int>> rle_y = run_length_encoding(quantized_y);
		std::vector<std::pair<int, int>> rle_cb = run_length_encoding(quantized_cb);
		std::vector<std::pair<int, int>> rle_cr = run_length_encoding(quantized_cr);

		HuffmanNode* root_y = build_huffman_tree(rle_y);
		HuffmanNode* root_cb = build_huffman_tree(rle_cb);
		HuffmanNode* root_cr = build_huffman_tree(rle_cr);

		std::string encoded_data_y = encode_with_huffman(root_y, quantized_y);
		std::string encoded_data_cb = encode_with_huffman(root_cb, quantized_cb);
		std::string encoded_data_cr = encode_with_huffman(root_cr, quantized_cr);

		int y_pad = 0;
		int cb_pad = 0;
		int cr_pad = 0;

		while (encoded_data_y.length() % 8 != 0) 
		{
			encoded_data_y += '0';
			y_pad++;
		}
		while (encoded_data_cb.length() % 8 != 0)
		{
			encoded_data_cb += '0';
			cb_pad++;
		}
		while (encoded_data_cr.length() % 8 != 0)
		{
			encoded_data_cr += '0';
			cr_pad++;
		}

		HeaderInfo info = HeaderInfo(encoded_data_y.length(), encoded_data_cb.length(), encoded_data_cr.length(), rle_y.size(), rle_cb.size(), rle_cr.size(), m_pImageLayer->m_Image.m_nH, m_pImageLayer->m_Image.m_nW, y_pad, cb_pad, cr_pad);
		write_all(info, encoded_data_y, encoded_data_cb, encoded_data_cr, rle_y, rle_cb, rle_cr);

		decode_image(OutY, OutCb, OutCr);

		/*
		 *	Convert YCbCr to RGB and Apply to Image
		 */

		for (int y = 0; y < m_pImageLayer->m_Image.m_nH; y++)
		{
			for (int x = 0; x < m_pImageLayer->m_Image.m_nW; x++)
			{
				m_pImageLayer->m_ImageOut.m_Red[y][x] =
					max(0, min(OutY[y][x] * 1 + OutCr[y / 2][x / 2] * 1.402, 255));
				m_pImageLayer->m_ImageOut.m_Green[y][x] =
					max(0, min(
						OutY[y][x] * 1 +
						OutCb[y / 2][x / 2] * -0.34414 +
						OutCr[y / 2][x / 2] * -0.71414, 255));
				m_pImageLayer->m_ImageOut.m_Blue[y][x] =
					max(0, min(OutY[y][x] * 1 + OutCb[y / 2][x / 2] * 1.772, 255));
			}
		}

		/*
		 *	Calculate PSNR
		 */

		double Psnr = GetPsnr(
			m_pImageLayer->m_Image.m_Red,
			m_pImageLayer->m_Image.m_Green,
			m_pImageLayer->m_Image.m_Blue,
			m_pImageLayer->m_ImageOut.m_Red,
			m_pImageLayer->m_ImageOut.m_Green,
			m_pImageLayer->m_ImageOut.m_Blue,
			m_pImageLayer->m_Image.m_nW, m_pImageLayer->m_Image.m_nH);

		std::cout << "\n\n";
		std::cout << "PSNR : " << Psnr << std::endl;

		double ssim = compute_ssim(m_pImageLayer->m_Image.m_Red, m_pImageLayer->m_ImageOut.m_Red, m_pImageLayer->m_Image.m_nW, m_pImageLayer->m_Image.m_nH);
		std::cout << "SSIM : " << ssim << "\n";

		/*
		 *	Free Memory
		 */

		free_dmatrix(InputR, m_pImageLayer->m_Image.m_nH, m_pImageLayer->m_Image.m_nW);
		free_dmatrix(InputG, m_pImageLayer->m_Image.m_nH, m_pImageLayer->m_Image.m_nW);
		free_dmatrix(InputB, m_pImageLayer->m_Image.m_nH, m_pImageLayer->m_Image.m_nW);

		free_dmatrix(OutR, m_pImageLayer->m_Image.m_nH, m_pImageLayer->m_Image.m_nW);
		free_dmatrix(OutG, m_pImageLayer->m_Image.m_nH, m_pImageLayer->m_Image.m_nW);
		free_dmatrix(OutB, m_pImageLayer->m_Image.m_nH, m_pImageLayer->m_Image.m_nW);

		free_dmatrix(InputY, m_pImageLayer->m_Image.m_nH, m_pImageLayer->m_Image.m_nW);
		free_dmatrix(InputCb, m_pImageLayer->m_Image.m_nH / 2, m_pImageLayer->m_Image.m_nW / 2);
		free_dmatrix(InputCr, m_pImageLayer->m_Image.m_nH / 2, m_pImageLayer->m_Image.m_nW / 2);

		m_pImageLayer->DrawBackgroundImage();

		m_bKeyPressed['D'] = m_bKeyPressed['I'] = m_bKeyPressed['C']
			= m_bKeyPressed['E'] = m_bKeyPressed['M'] = m_bKeyPressed['Y'] = false;
	}

	m_pScene->Render();
	DrawSceneTextPos("Image Processing", CKgPoint(0, 0));
	
	CKhuGleWin::Update();
}

int main()
{
	char ExePath[MAX_PATH], ImagePath[MAX_PATH];

	GetModuleFileName(NULL, ExePath, MAX_PATH);

	int i;
	int LastBackSlash = -1;
	int nLen = strlen(ExePath);
	for(i = nLen-1 ; i >= 0 ; i--)
	{
		if(ExePath[i] == '\\') {
			LastBackSlash = i;
			break;
		}
	}

	if(LastBackSlash >= 0)
		ExePath[LastBackSlash] = '\0';

	/*
	 *	Add Image Name Input
	 */
	char ImageName[100];
	std::cout << "Enter Image Name (*.bmp) : ";
	std::cin >> ImageName;

	sprintf(ImagePath, "%s\\%s", ExePath, ImageName);
	/*
	 *	Add Image Name Input End
	 */

	CImageProcessing *pImageProcessing = new CImageProcessing(640, 480, ImagePath);
	KhuGleWinInit(pImageProcessing);

	return 0;
}