#pragma once

#include "DWT.h"
#include "Huffman.h"
#include "Reader.h"

inline void decompress_image(const char* read_path, double** OutY, double** OutCb, double** OutCr)
{
	const auto result = read_all(read_path);
	const HeaderInfo info = std::get<0>(result);

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

	std::string r_encoded_data_y = std::get<1>(result)[0];
	std::string r_encoded_data_cb = std::get<1>(result)[1];
	std::string r_encoded_data_cr = std::get<1>(result)[2];

	r_encoded_data_y.erase(r_encoded_data_y.length() - info.y_pad, info.y_pad);
	r_encoded_data_cb.erase(r_encoded_data_cb.length() - info.cb_pad, info.cb_pad);
	r_encoded_data_cr.erase(r_encoded_data_cr.length() - info.cr_pad, info.cr_pad);

	std::vector<std::pair<int, int>> r_rle_y = std::get<2>(result)[0];
	std::vector<std::pair<int, int>> r_rle_cb = std::get<2>(result)[1];
	std::vector<std::pair<int, int>> r_rle_cr = std::get<2>(result)[2];

	HuffmanNode* r_root_y = build_huffman_tree(r_rle_y);
	HuffmanNode* r_root_cb = build_huffman_tree(r_rle_cb);
	HuffmanNode* r_root_cr = build_huffman_tree(r_rle_cr);

	std::vector<std::vector<int>> decoded_data_y = decode_with_huffman(r_encoded_data_y, r_root_y, info.w);
	std::vector<std::vector<int>> decoded_data_cb = decode_with_huffman(r_encoded_data_cb, r_root_cb, info.w / 2);
	std::vector<std::vector<int>> decoded_data_cr = decode_with_huffman(r_encoded_data_cr, r_root_cr, info.w / 2);

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

	iwt2D(OutY, h_half);
	iwt2D(OutCb, h_qut);
	iwt2D(OutCr, h_qut);

	iwt2D(OutY, h);
	iwt2D(OutCb, h_half);
	iwt2D(OutCr, h_half);
}
