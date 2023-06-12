#pragma once

#include "DWT.h"
#include "Huffman.h"
#include "Reader.h"

inline void decompress_image(const CompResult* result, double** OutY, double** OutCb, double** OutCr, bool visualize = false, std::vector<CKhuGleSignal>& out_image_vec = std::vector<CKhuGleSignal>())
{
	const HeaderInfo info = result->info;

	int h = info.h;
	int w = info.w;
	int h_half = info.h / 2;
	int w_half = info.w / 2;
	int h_qut = info.h / 4;
	int w_qut = info.w / 4;

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

	std::string r_encoded_data_y = result->encoded_data_y;
	std::string r_encoded_data_cb = result->encoded_data_cb;
	std::string r_encoded_data_cr = result->encoded_data_cr;

	r_encoded_data_y.erase(r_encoded_data_y.length() - info.y_pad, info.y_pad);
	r_encoded_data_cb.erase(r_encoded_data_cb.length() - info.cb_pad, info.cb_pad);
	r_encoded_data_cr.erase(r_encoded_data_cr.length() - info.cr_pad, info.cr_pad);

	std::vector<std::pair<int, int>> r_rle_y = result->rle_y;
	std::vector<std::pair<int, int>> r_rle_cb = result->rle_cb;
	std::vector<std::pair<int, int>> r_rle_cr = result->rle_cr;

	HuffmanNode* r_root_y = build_huffman_tree(r_rle_y);
	HuffmanNode* r_root_cb = build_huffman_tree(r_rle_cb);
	HuffmanNode* r_root_cr = build_huffman_tree(r_rle_cr);

	std::vector<std::vector<int>> decoded_data_y = decode_with_huffman(r_encoded_data_y, r_root_y, info.w);
	std::vector<std::vector<int>> decoded_data_cb = decode_with_huffman(r_encoded_data_cb, r_root_cb, info.w / 2);
	std::vector<std::vector<int>> decoded_data_cr = decode_with_huffman(r_encoded_data_cr, r_root_cr, info.w / 2);

	std::vector<double> step_size_vec;

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

		step_size_vec.push_back(step_size);

		auto h_range = sub_band.first;
		auto w_range = sub_band.second;

		for (int y = h_range.first; y < h_range.second; y++)
		{
			for (int x = w_range.first; x < w_range.second; x++)
			{
				OutY[y][x] = decoded_data_y[y][x] * step_size;
			}
		}

		for (int y = h_range.first / 2; y < h_range.second / 2; y++)
		{
			for (int x = w_range.first / 2; x < w_range.second / 2; x++)
			{
				OutCb[y][x] = decoded_data_cb[y][x] * step_size;
				OutCr[y][x] = decoded_data_cr[y][x] * step_size;
			}
		}

		index++;
	}

	if (visualize)
	{
		out_image_vec[0].m_Red = cmatrix(h, w);
		out_image_vec[0].m_Green = cmatrix(h, w);
		out_image_vec[0].m_Blue = cmatrix(h, w);

		out_image_vec[0].m_nH = h;
		out_image_vec[0].m_nW = w;

		auto len_y = r_encoded_data_y.length() / 8;
		auto len_cb = r_encoded_data_cb.length() / 8;
		auto len_cr = r_encoded_data_cr.length() / 8;

		auto y_y = len_y / h;
		auto y_x = len_y % h;

		auto cb_y = len_cb / h;
		auto cb_x = len_cb % h;

		auto cr_y = len_cr / h;
		auto cr_x = len_cr % h;

		for (int y = 0; y < h; y++)
		{
			for (int x = 0; x < w; x++)
			{
				out_image_vec[0].m_Red[y][x] = 0;
				out_image_vec[0].m_Green[y][x] = 0;
				out_image_vec[0].m_Blue[y][x] = 0;
			}
		}

		for (int y = 0; y < y_y; y++)
		{
			auto max_x = (y == y_y - 1) ? y_x : w;
			for (int x = 0; x < max_x; x++)
			{
				out_image_vec[0].m_Red[y][x] += 76;
				out_image_vec[0].m_Green[y][x] += 149;
				out_image_vec[0].m_Blue[y][x] += 29;
			}
		}

		for (int y = 0; y < cb_y; y++)
		{
			auto max_x = (y == cb_y - 1) ? cb_x : w;
			for (int x = 0; x < max_x; x++)
			{
				out_image_vec[0].m_Red[y][x] -= 43;
				out_image_vec[0].m_Green[y][x] -= 84;
				out_image_vec[0].m_Blue[y][x] += 127;
			}
		}

		for (int y = 0; y < cr_y; y++)
		{
			auto max_x = (y == cr_y - 1) ? cr_x : w;
			for (int x = 0; x < max_x; x++)
			{
				out_image_vec[0].m_Red[y][x] += 127;
				out_image_vec[0].m_Green[y][x] -= 106;
				out_image_vec[0].m_Blue[y][x] -= 20;
			}
		}

		out_image_vec[1].m_Red = cmatrix(h, w);
		out_image_vec[1].m_Green = cmatrix(h, w);
		out_image_vec[1].m_Blue = cmatrix(h, w);

		out_image_vec[1].m_nH = h;
		out_image_vec[1].m_nW = w;

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
					out_image_vec[1].m_Red[y][x] = n;
					out_image_vec[1].m_Green[y][x] = 0;
					out_image_vec[1].m_Blue[y][x] = 0;
				}
			}

			index++;
		}

		out_image_vec[2].m_Red = cmatrix(h, w);
		out_image_vec[2].m_Green = cmatrix(h, w);
		out_image_vec[2].m_Blue = cmatrix(h, w);

		out_image_vec[2].m_nH = h;
		out_image_vec[2].m_nW = w;

		double** fwt_r = dmatrix(h, w);
		double** fwt_g = dmatrix(h, w);
		double** fwt_b = dmatrix(h, w);

		YCbCr2RGB(OutY, OutCb, OutCr, fwt_r, fwt_g, fwt_b, h, w);

		for (int y = 0; y < h; y++)
		{
			for (int x = 0; x < w; x++)
			{
				out_image_vec[2].m_Red[y][x] = fwt_r[y][x];
				out_image_vec[2].m_Green[y][x] = fwt_g[y][x];
				out_image_vec[2].m_Blue[y][x] = fwt_b[y][x];
			}
		}

		free_dmatrix(fwt_r, h, w);
		free_dmatrix(fwt_g, h, w);
		free_dmatrix(fwt_b, h, w);
	}

	iwt2D(OutY, h_half);
	iwt2D(OutCb, h_qut);
	iwt2D(OutCr, h_qut);

	iwt2D(OutY, h);
	iwt2D(OutCb, h_half);
	iwt2D(OutCr, h_half);
}

inline void decompress_image(const char* read_path, double** OutY, double** OutCb, double** OutCr, std::vector<CKhuGleSignal>& out_image_vec)
{
	decompress_image(read_all(read_path), OutY, OutCb, OutCr, true, out_image_vec);
}
