#pragma once

#include "DWT.h"
#include "Huffman.h"
#include "Reader.h"

inline void DecompressImage(const CompResult* result, double** OutY, double** OutCb, double** OutCr,
                            bool visualize = false,
                            std::vector<CKhuGleSignal>& out_image_vec = std::vector<CKhuGleSignal>(),
                            std::string& str = std::string(""), std::string& str2 = std::string(""))
{
	const HeaderInfo info = result->info_;

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

	std::string r_encoded_data_y = result->encoded_data_y_;
	std::string r_encoded_data_cb = result->encoded_data_cb_;
	std::string r_encoded_data_cr = result->encoded_data_cr_;

	r_encoded_data_y.erase(r_encoded_data_y.length() - info.y_pad, info.y_pad);
	r_encoded_data_cb.erase(r_encoded_data_cb.length() - info.cb_pad, info.cb_pad);
	r_encoded_data_cr.erase(r_encoded_data_cr.length() - info.cr_pad, info.cr_pad);

	std::vector<std::pair<int, int>> r_rle_y = result->rle_y_;
	std::vector<std::pair<int, int>> r_rle_cb = result->rle_cb_;
	std::vector<std::pair<int, int>> r_rle_cr = result->rle_cr_;

	HuffmanNode* r_root_y = BuildHuffmanTree(r_rle_y);
	HuffmanNode* r_root_cb = BuildHuffmanTree(r_rle_cb);
	HuffmanNode* r_root_cr = BuildHuffmanTree(r_rle_cr);

	std::vector<std::vector<int>> decoded_data_y = DecodeWithHuffman(r_encoded_data_y, r_root_y, info.w);
	std::vector<std::vector<int>> decoded_data_cb = DecodeWithHuffman(r_encoded_data_cb, r_root_cb, info.w / 2);
	std::vector<std::vector<int>> decoded_data_cr = DecodeWithHuffman(r_encoded_data_cr, r_root_cr, info.w / 2);

	std::vector<double> step_size_vec;
	double** step_size_2d = dmatrix(h, w);

	int index = 0;
	for (auto sub_band : map)
	{
		double r = info.deep ? 9 : 8;
		double i = 2;
		double c = 8;
		double f = info.deep ? 230 : 23;

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

		int alpha = h_range.first + w_range.first;
		int beta = h_range.second + w_range.second;
		double a = 1.5 / double(beta - alpha);
		double b = 0.5 - double(a * alpha);

		for (int y = h_range.first; y < h_range.second; y++)
		{
			for (int x = w_range.first; x < w_range.second; x++)
			{
				double mul = a * (x + y) + b;
				step_size_2d[y][x] = step_size * (info.deep ? mul : 1);

				OutY[y][x] = decoded_data_y[y][x] * (step_size * (info.deep ? mul : 1));
			}
		}

		alpha = h_range.first / 2 + w_range.first / 2;
		beta = h_range.second / 2 + w_range.second / 2;
		a = 1.5 / double(beta - alpha);
		b = 0.5 - double(a * alpha);

		for (int y = h_range.first / 2; y < h_range.second / 2; y++)
		{
			for (int x = w_range.first / 2; x < w_range.second / 2; x++)
			{
				double mul = a * (x + y) + b;

				OutCb[y][x] = decoded_data_cb[y][x] * (step_size * (info.deep ? mul : 1));
				OutCr[y][x] = decoded_data_cr[y][x] * (step_size * (info.deep ? mul : 1));
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

		auto f = Flatten(step_size_2d, 0, h, 0, w);
		double max_step_size_2d = *std::max_element(std::begin(f), std::end(f));
		double min_step_size_2d = *std::min_element(std::begin(f), std::end(f));

		int index = 0;
		for (auto sub_band : map)
		{
			auto h_range = sub_band.first;
			auto w_range = sub_band.second;

			for (int y = h_range.first; y < h_range.second; y++)
			{
				for (int x = w_range.first; x < w_range.second; x++)
				{
					double n = 0.0;
					if (info.deep)
					{
						n = (step_size_2d[y][x] - min_step_size_2d) / max_step_size_2d;
					}
					else
					{
						n = (step_size_vec[index] - min_step_size) / max_step_size;
					}

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

		char buffer[100];
		sprintf(buffer, "Step Size (min, max) : %.3f, %.3f", info.deep ? min_step_size_2d : min_step_size,
		        info.deep ? max_step_size_2d : max_step_size);
		str = std::string(buffer);

		sprintf(buffer, "RLE Size (Y, Cb, Cr) : %d, %d, %d", r_rle_y.size(), r_rle_cb.size(), r_rle_cr.size());
		str2 = std::string(buffer);
	}

	IWT2D(OutY, h_half);
	IWT2D(OutCb, h_qut);
	IWT2D(OutCr, h_qut);

	IWT2D(OutY, h);
	IWT2D(OutCb, h_half);
	IWT2D(OutCr, h_half);

	delete step_size_2d;
}

inline void DecompressImage(const char* read_path, double** OutY, double** OutCb, double** OutCr,
                            std::vector<CKhuGleSignal>& out_image_vec, std::string& str, std::string& str2)
{
	DecompressImage(ReadAll(read_path), OutY, OutCb, OutCr, true, out_image_vec, str, str2);
}
