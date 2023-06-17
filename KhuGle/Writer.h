#pragma once

#include <bitset>
#include <fstream>
#include <string>
#include <vector>
#include "Type.h"

inline void WriteHeader(std::ofstream& stream, const HeaderInfo& info)
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
	stream.write(reinterpret_cast<const char*>(&info.lvl), sizeof(info.lvl));
}

inline void WriteImgData(std::ofstream& stream, const std::string& image_data)
{
	for (int i = 0; i < image_data.length(); i += 8)
	{
		char byte = static_cast<char>(std::bitset<8>(image_data.substr(i, 8)).to_ulong());
		stream.write(&byte, sizeof(byte));
	}
}

inline void WriteRLEData(std::ofstream& stream, const std::vector<std::pair<int, int>>& rle)
{
	for (const auto& pair : rle)
	{
		stream.write(reinterpret_cast<const char*>(&pair.first), sizeof(pair.first));
		stream.write(reinterpret_cast<const char*>(&pair.second), sizeof(pair.second));
	}
}

inline void WriteAll(const char* write_path, CompResult* result)
{
	std::ofstream fout(write_path, std::ios::out | std::ios::binary);
	if (!fout) return;

	const auto info = result->info_;

	const auto y_image_data = result->encoded_data_y_;
	const auto cb_image_data = result->encoded_data_cb_;
	const auto cr_image_data = result->encoded_data_cr_;

	const auto y_rle_data = result->rle_y_;
	const auto cb_rle_data = result->rle_cb_;
	const auto cr_rle_data = result->rle_cr_;

	WriteHeader(fout, info);

	WriteImgData(fout, y_image_data);
	WriteImgData(fout, cb_image_data);
	WriteImgData(fout, cr_image_data);

	WriteRLEData(fout, y_rle_data);
	WriteRLEData(fout, cb_rle_data);
	WriteRLEData(fout, cr_rle_data);

	fout.close();
}
