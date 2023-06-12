#pragma once

#include "Writer.h"

inline HeaderInfo read_header(std::ifstream& stream)
{
	HeaderInfo info = HeaderInfo();

	stream.read(reinterpret_cast<char*>(&info.y_len), sizeof(info.y_len));
	stream.read(reinterpret_cast<char*>(&info.cb_len), sizeof(info.cb_len));
	stream.read(reinterpret_cast<char*>(&info.cr_len), sizeof(info.cr_len));
	stream.read(reinterpret_cast<char*>(&info.rle_y_len), sizeof(info.rle_y_len));
	stream.read(reinterpret_cast<char*>(&info.rle_cb_len), sizeof(info.rle_cb_len));
	stream.read(reinterpret_cast<char*>(&info.rle_cr_len), sizeof(info.rle_cr_len));
	stream.read(reinterpret_cast<char*>(&info.h), sizeof(info.h));
	stream.read(reinterpret_cast<char*>(&info.w), sizeof(info.w));
	stream.read(reinterpret_cast<char*>(&info.y_pad), sizeof(info.y_pad));
	stream.read(reinterpret_cast<char*>(&info.cb_pad), sizeof(info.cb_pad));
	stream.read(reinterpret_cast<char*>(&info.cr_pad), sizeof(info.cr_pad));

	return info;
}

inline std::string read_image_data(std::ifstream& stream, int size)
{
	std::vector<char> buffer(size / 8);
	stream.read(buffer.data(), size / 8);

	std::string binary_string;
	for (char c : buffer)
	{
		binary_string += std::bitset<8>(static_cast<unsigned char>(c)).to_string();
	}

	return binary_string;
}

inline std::vector<std::pair<int, int>> read_rle_data(std::ifstream& stream, int size)
{
	std::vector<std::pair<int, int>> rle;

	for (int i = 0; i < size; i++)
	{
		std::pair<int, int> pair;

		stream.read(reinterpret_cast<char*>(&pair.first), sizeof(pair.first));
		stream.read(reinterpret_cast<char*>(&pair.second), sizeof(pair.second));

		rle.push_back(pair);
	}

	return rle;
}

inline CompResult* read_all(const char* read_path)
{
	std::ifstream fin(read_path, std::ios::in | std::ios::binary);
	if (!fin) return {};

	const HeaderInfo info = read_header(fin);

	const std::string encoded_data_y = read_image_data(fin, info.y_len);
	const std::string encoded_data_cb = read_image_data(fin, info.cb_len);
	const std::string encoded_data_cr = read_image_data(fin, info.cr_len);

	const std::vector<std::pair<int, int>> rle_y = read_rle_data(fin, info.rle_y_len);
	const std::vector<std::pair<int, int>> rle_cb = read_rle_data(fin, info.rle_cb_len);
	const std::vector<std::pair<int, int>> rle_cr = read_rle_data(fin, info.rle_cr_len);

	fin.close();

	return new CompResult{ info, encoded_data_y, encoded_data_cb, encoded_data_cr, rle_y, rle_cb, rle_cr };
}

inline int measure_file_size(const char* path)
{
	std::ifstream fin(path, std::ios::in | std::ios::binary);
	if (!fin) return -1;

	fin.seekg(0, std::ios::end);
	const int file_size = fin.tellg();
	fin.close();

	return file_size;
}

inline int measure_file_size(CompResult* result)
{
	write_all("Temp.comp", result);
	const int file_size = measure_file_size("Temp.comp");
	unlink("Temp.comp");

	return file_size;
}