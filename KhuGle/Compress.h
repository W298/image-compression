#pragma once

#include "Colorspace.h"
#include "DWT.h"
#include "Huffman.h"
#include "Writer.h"

#include <iostream>

inline std::vector<int> flatten(const std::vector<std::vector<int>>& origin, int h_start, int h_end, int w_start,
                                int w_end)
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

inline std::vector<double> flatten(double** input, int h_start, int h_end, int w_start, int w_end)
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

inline std::map<double, int> compute_precise_histogram(const std::vector<double>& dwt_coefficients)
{
	std::map<double, int> precise_histogram;
	for (auto coeff : dwt_coefficients)
	{
		double symbol = std::round(coeff * 1000000) / 1000000;
		precise_histogram[symbol]++;
	}
	return precise_histogram;
}

inline std::map<double, double> compute_precise_quantization_table(const std::map<double, int>& precise_histogram,
                                                                   double base_step_size)
{
	std::map<double, double> q_table;

	for (const std::pair<double, int> value : precise_histogram)
	{
		double symbol = value.first;
		int count = value.second;
		q_table[symbol] = base_step_size / std::sqrt(count);
	}

	return q_table;
}

inline std::map<int, int> compute_histogram(const std::vector<double>& dwt_coefficients)
{
	std::map<int, int> histogram;

	for (auto coeff : dwt_coefficients)
	{
		int bin = std::round(coeff);
		histogram[bin]++;
	}

	return histogram;
}

inline std::map<int, int> compute_histogram(const std::vector<int>& quantized_values)
{
	std::map<int, int> histogram;

	for (auto q : quantized_values)
	{
		histogram[q]++;
	}

	return histogram;
}

inline std::map<int, double> compute_quantization_table(const std::map<int, int>& histogram, double base_step_size)
{
	std::map<int, double> q_table;

	for (const std::pair<int, int> value : histogram)
	{
		int bin = value.first;
		int count = value.second;
		q_table[bin] = base_step_size / std::sqrt(count);
	}

	return q_table;
}

inline void compress_image(const char* write_path, double** InputY, double** InputCb, double** InputCr, int img_height, int img_width, std::vector<CKhuGleSignal>& out_image_vec)
{
	constexpr bool use_dynamic_table = false;

	fwt2D(InputY, img_height);
	fwt2D(InputCb, img_height / 2);
	fwt2D(InputCr, img_height / 2);

	fwt2D(InputY, img_height / 2);
	fwt2D(InputCb, img_height / 4);
	fwt2D(InputCr, img_height / 4);

	out_image_vec[1].m_Red = cmatrix(img_height, img_width);
	out_image_vec[1].m_Green = cmatrix(img_height, img_width);
	out_image_vec[1].m_Blue = cmatrix(img_height, img_width);

	out_image_vec[1].m_nH = img_height;
	out_image_vec[1].m_nW = img_width;

	double** fwt_r = dmatrix(img_height, img_width);
	double** fwt_g = dmatrix(img_height, img_width);
	double** fwt_b = dmatrix(img_height, img_width);

	YCbCr2RGB(InputY, InputCb, InputCr, fwt_r, fwt_g, fwt_b, img_height, img_width);

	for (int y = 0; y < img_height; y++)
	{
		for (int x = 0; x < img_width; x++)
		{
			out_image_vec[1].m_Red[y][x] = fwt_r[y][x];
			out_image_vec[1].m_Green[y][x] = fwt_g[y][x];
			out_image_vec[1].m_Blue[y][x] = fwt_b[y][x];
		}
	}

	free_dmatrix(fwt_r, img_height, img_width);
	free_dmatrix(fwt_g, img_height, img_width);
	free_dmatrix(fwt_b, img_height, img_width);

	int h = img_height;
	int w = img_width;
	int h_half = img_height / 2;
	int w_half = img_width / 2;
	int h_qut = img_height / 4;
	int w_qut = img_width / 4;

	std::vector<std::string> map_name = {"LL1", "LH1", "HL1", "HH1", "LH", "HL", "HH"};
	std::vector<std::pair<std::pair<int, int>, std::pair<int, int>>> map;
	std::vector<double> factor = {2, 8, 8, 32, 32, 128, 512};

	map.push_back({{0, h_qut}, {0, w_qut}});
	map.push_back({{0, h_qut}, {w_qut, w_half}});
	map.push_back({{h_qut, h_half}, {0, w_qut}});
	map.push_back({{h_qut, h_half}, {w_qut, w_half}});
	map.push_back({{0, h_half}, {w_half, w}});
	map.push_back({{h_half, h}, {0, w_half}});
	map.push_back({{h_half, h}, {w_half, w}});

	std::vector<double> step_size_vec;

	std::vector<std::vector<int>> quantized_y(img_height, std::vector<int>(img_width));
	std::vector<std::vector<int>> quantized_cb(img_height / 2, std::vector<int>(img_width / 2));
	std::vector<std::vector<int>> quantized_cr(img_height / 2, std::vector<int>(img_width / 2));

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
			step_size_vec.push_back(step_size);

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
				}
			}

			for (int y = h_range.first / 2; y < h_range.second / 2; y++)
			{
				for (int x = w_range.first / 2; x < w_range.second / 2; x++)
				{
					int q = (int)std::round(InputCb[y][x] / step_size);
					quantized_cb[y][x] = q;

					q = (int)std::round(InputCr[y][x] / step_size);
					quantized_cr[y][x] = q;
				}
			}

			index++;
		}

		for (int i = 0; i < map.size(); i++)
		{
			auto histogram = compute_precise_histogram(flatten(InputY, map[i].first.first, map[i].first.second,
			                                                   map[i].second.first, map[i].second.second));
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
			auto flatted_vector_cb = flatten(InputCb, h_range.first / 2, h_range.second / 2, w_range.first / 2,
			                                 w_range.second / 2);
			auto flatted_vector_cr = flatten(InputCr, h_range.first / 2, h_range.second / 2, w_range.first / 2,
			                                 w_range.second / 2);

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
			auto histogram = compute_precise_histogram(flatten(InputY, map[i].first.first, map[i].first.second,
			                                                   map[i].second.first, map[i].second.second));
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
			auto flatted_vector_cb = flatten(quantized_cb, h_range.first / 2, h_range.second / 2, w_range.first / 2,
			                                 w_range.second / 2);
			auto flatted_vector_cr = flatten(quantized_cr, h_range.first / 2, h_range.second / 2, w_range.first / 2,
			                                 w_range.second / 2);

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

	out_image_vec[2].m_Red = cmatrix(img_height, img_width);
	out_image_vec[2].m_Green = cmatrix(img_height, img_width);
	out_image_vec[2].m_Blue = cmatrix(img_height, img_width);

	out_image_vec[2].m_nH = img_height;
	out_image_vec[2].m_nW = img_width;

	double max_step_size = *std::max_element(std::begin(step_size_vec), std::end(step_size_vec));
	double min_step_size = *std::min_element(std::begin(step_size_vec), std::end(step_size_vec));

	int index = 0;
	for (auto sub_band : map)
	{
		auto h_range = sub_band.first;
		auto w_range = sub_band.second;

		for (int y = h_range.first; y < h_range.second; y++)
		{
			for (int x = w_range.first; x < w_range.second; x++)
			{
				double n = (step_size_vec[index] - min_step_size) / max_step_size;
				n *= 255;
				out_image_vec[2].m_Red[y][x] = n;
				out_image_vec[2].m_Green[y][x] = 0;
				out_image_vec[2].m_Blue[y][x] = 0;
			}
		}

		index++;
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

	out_image_vec[3].m_Red = cmatrix(img_height, img_width);
	out_image_vec[3].m_Green = cmatrix(img_height, img_width);
	out_image_vec[3].m_Blue = cmatrix(img_height, img_width);

	out_image_vec[3].m_nH = img_height;
	out_image_vec[3].m_nW = img_width;

	auto len_y = encoded_data_y.length() / 8;
	auto len_cb = encoded_data_cb.length() / 8;
	auto len_cr = encoded_data_cr.length() / 8;

	auto y_y = len_y / img_height;
	auto y_x = len_y % img_height;

	auto cb_y = len_cb / img_height;
	auto cb_x = len_cb % img_height;

	auto cr_y = len_cr / img_height;
	auto cr_x = len_cr % img_height;

	for (int y = 0; y < img_height; y++)
	{
		for (int x = 0; x < img_width; x++)
		{
			out_image_vec[3].m_Red[y][x] = 0;
			out_image_vec[3].m_Green[y][x] = 0;
			out_image_vec[3].m_Blue[y][x] = 0;
		}
	}

	for (int y = 0; y < y_y; y++)
	{
		auto max_x = (y == y_y - 1) ? y_x : img_width;
		for (int x = 0; x < max_x; x++)
		{
			out_image_vec[3].m_Red[y][x] += 76;
			out_image_vec[3].m_Green[y][x] += 149;
			out_image_vec[3].m_Blue[y][x] += 29;
		}
	}

	for (int y = 0; y < cb_y; y++)
	{
		auto max_x = (y == cb_y - 1) ? cb_x : img_width;
		for (int x = 0; x < max_x; x++)
		{
			out_image_vec[3].m_Red[y][x] -= 43;
			out_image_vec[3].m_Green[y][x] -= 84;
			out_image_vec[3].m_Blue[y][x] += 127;
		}
	}

	for (int y = 0; y < cr_y; y++)
	{
		auto max_x = (y == cr_y - 1) ? cr_x : img_width;
		for (int x = 0; x < max_x; x++)
		{
			out_image_vec[3].m_Red[y][x] += 127;
			out_image_vec[3].m_Green[y][x] -= 106;
			out_image_vec[3].m_Blue[y][x] -= 20;
		}
	}

	HeaderInfo info = HeaderInfo(
		encoded_data_y.length(),
		encoded_data_cb.length(),
		encoded_data_cr.length(),
		rle_y.size(),
		rle_cb.size(),
		rle_cr.size(),
		img_height, img_width,
		y_pad, cb_pad, cr_pad
	);

	write_all(write_path, info,
	          encoded_data_y,
	          encoded_data_cb,
	          encoded_data_cr,
	          rle_y,
	          rle_cb,
	          rle_cr
	);
}
