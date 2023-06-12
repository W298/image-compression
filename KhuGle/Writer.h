#pragma once

#include <bitset>
#include <fstream>
#include <string>
#include <vector>
#include "Type.h"

inline void write_header(std::ofstream& stream, const HeaderInfo& info)
{
	stream.write(reinterpret_cast<const char*>(&info.y_len), sizeof(info.y_len));
	stream.write(reinterpret_cast<const char*>(&info.cb_len), sizeof(info.cb_len));
	stream.write(reinterpret_cast<const char*>(&info.cr_len), sizeof(info.cr_len));
	stream.write(reinterpret_cast<const char*>(&info.rle_y_len), sizeof(info.rle_y_len));
	stream.write(reinterpret_cast<const char*>(&info.rle_cb_len), sizeof(info.rle_cb_len));
	stream.write(reinterpret_cast<const char*>(&info.rle_cr_len), sizeof(info.rle_cr_len));
	stream.write(reinterpret_cast<const char*>(&info.h), sizeof(info.h));
	stream.write(reinterpret_cast<const char*>(&info.w), sizeof(info.w));
	stream.write(reinterpret_cast<const char*>(&info.y_pad), sizeof(info.y_pad));
	stream.write(reinterpret_cast<const char*>(&info.cb_pad), sizeof(info.cb_pad));
	stream.write(reinterpret_cast<const char*>(&info.cr_pad), sizeof(info.cr_pad));
}

inline void write_image_data(std::ofstream& stream, const std::string& image_data)
{
	for (int i = 0; i < image_data.length(); i += 8)
	{
		char byte = static_cast<char>(std::bitset<8>(image_data.substr(i, 8)).to_ulong());
		stream.write(&byte, sizeof(byte));
	}
}

inline void write_rle_data(std::ofstream& stream, const std::vector<std::pair<int, int>>& rle)
{
	for (const auto& pair : rle)
	{
		stream.write(reinterpret_cast<const char*>(&pair.first), sizeof(pair.first));
		stream.write(reinterpret_cast<const char*>(&pair.second), sizeof(pair.second));
	}
}

inline void write_all(const char* write_path, CompResult* result)
{
	std::ofstream fout(write_path, std::ios::out | std::ios::binary);
	if (!fout) return;

	const auto info = result->info;

	const auto y_image_data = result->encoded_data_y;
	const auto cb_image_data = result->encoded_data_cb;
	const auto cr_image_data = result->encoded_data_cr;

	const auto y_rle_data = result->rle_y;
	const auto cb_rle_data = result->rle_cb;
	const auto cr_rle_data = result->rle_cr;

	write_header(fout, info);

	write_image_data(fout, y_image_data);
	write_image_data(fout, cb_image_data);
	write_image_data(fout, cr_image_data);

	write_rle_data(fout, y_rle_data);
	write_rle_data(fout, cb_rle_data);
	write_rle_data(fout, cr_rle_data);

	fout.close();
}
