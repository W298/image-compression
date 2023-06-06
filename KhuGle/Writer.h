#pragma once

#include <bitset>
#include <fstream>
#include <string>
#include <vector>

struct HeaderInfo
{
	int y_len, cb_len, cr_len;
	int rle_y_len, rle_cb_len, rle_cr_len;
	int h, w;
	int y_pad, cb_pad, cr_pad;

	HeaderInfo(): y_len(0), cb_len(0),
	              cr_len(0), rle_y_len(0),
	              rle_cb_len(0),
	              rle_cr_len(0), h(0), w(0), y_pad(0), cb_pad(0), cr_pad(0)
	{
	}

	HeaderInfo(int y_len, int cb_len, int cr_len, int rle_y_len, int rle_cb_len, int rle_cr_len, int w, int h,
	           int y_pad, int cb_pad, int cr_pad) : y_len(y_len),
	                                                cb_len(cb_len),
	                                                cr_len(cr_len), rle_y_len(rle_y_len),
	                                                rle_cb_len(rle_cb_len),
	                                                rle_cr_len(rle_cr_len), h(h), w(w), y_pad(y_pad), cb_pad(cb_pad),
	                                                cr_pad(cr_pad)
	{
	}
};

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

inline void write_all(const char* write_path, const HeaderInfo& info, const std::string& y_image_data, const std::string& cb_image_data,
                      const std::string& cr_image_data,
                      const std::vector<std::pair<int, int>>& y_rle_data,
                      const std::vector<std::pair<int, int>>& cb_rle_data,
                      const std::vector<std::pair<int, int>>& cr_rle_data)
{
	std::ofstream fout(write_path, std::ios::out | std::ios::binary);
	if (!fout) return;

	write_header(fout, info);

	write_image_data(fout, y_image_data);
	write_image_data(fout, cb_image_data);
	write_image_data(fout, cr_image_data);

	write_rle_data(fout, y_rle_data);
	write_rle_data(fout, cb_rle_data);
	write_rle_data(fout, cr_rle_data);

	fout.close();
}
