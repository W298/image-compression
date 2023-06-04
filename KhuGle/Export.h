#pragma once

#include <bitset>
#include <fstream>
#include <string>

using namespace std;

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

	HeaderInfo(int y_len, int cb_len, int cr_len, int rle_y_len, int rle_cb_len, int rle_cr_len, int w, int h, int y_pad, int cb_pad, int cr_pad) : y_len(y_len),
		cb_len(cb_len),
		cr_len(cr_len), rle_y_len(rle_y_len),
		rle_cb_len(rle_cb_len),
		rle_cr_len(rle_cr_len), h(h), w(w), y_pad(y_pad), cb_pad(cb_pad), cr_pad(cr_pad)
	{
	}
};

void write_header(ofstream& stream, const HeaderInfo& info)
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

void write_image_data(ofstream& stream, const string& image_data)
{
	for (int i = 0; i < image_data.length(); i += 8)
	{
		char byte = static_cast<char>(bitset<8>(image_data.substr(i, 8)).to_ulong());
		stream.write(&byte, sizeof(byte));
	}
}

void write_rle_data(ofstream& stream, const std::vector<std::pair<int, int>>& rle)
{
	for (const auto& pair : rle)
	{
		stream.write(reinterpret_cast<const char*>(&pair.first), sizeof(pair.first));
		stream.write(reinterpret_cast<const char*>(&pair.second), sizeof(pair.second));
	}
}

void write_all(const HeaderInfo& info, const string& y_image_data, const string& cb_image_data,
               const string& cr_image_data,
               const std::vector<std::pair<int, int>>& y_rle_data, const std::vector<std::pair<int, int>>& cb_rle_data,
               const std::vector<std::pair<int, int>>& cr_rle_data)
{
	ofstream fout("..\\Run\\example.comp", ios::out | ios::binary);
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

HeaderInfo read_header(ifstream& stream)
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

string read_image_data(ifstream& stream, int size)
{
	vector<char> buffer(size / 8);
	stream.read(buffer.data(), size / 8);

	std::string binary_string;
	for (char c : buffer)
	{
		binary_string += std::bitset<8>(static_cast<unsigned char>(c)).to_string();
	}

	return binary_string;
}

vector<pair<int, int>> read_rle_data(ifstream& stream, int size)
{
	std::vector<std::pair<int, int>> rle;

	for (int i = 0; i < size; i++)
	{
		pair<int, int> pair;

		stream.read(reinterpret_cast<char*>(&pair.first), sizeof(pair.first));
		stream.read(reinterpret_cast<char*>(&pair.second), sizeof(pair.second));

		rle.push_back(pair);
	}

	return rle;
}

tuple<HeaderInfo, vector<string>, vector<vector<pair<int, int>>>> read_all()
{
	ifstream fin("..\\Run\\example.comp", ios::in | ios::binary);
	if (!fin) return {};

	const HeaderInfo info = read_header(fin);

	const string encoded_data_y = read_image_data(fin, info.y_len);
	const string encoded_data_cb = read_image_data(fin, info.cb_len);
	const string encoded_data_cr = read_image_data(fin, info.cr_len);

	const vector<pair<int, int>> rle_y = read_rle_data(fin, info.rle_y_len);
	const vector<pair<int, int>> rle_cb = read_rle_data(fin, info.rle_cb_len);
	const vector<pair<int, int>> rle_cr = read_rle_data(fin, info.rle_cr_len);

	fin.close();

	vector<string> str_vec = { encoded_data_y, encoded_data_cb, encoded_data_cr };
	vector<vector<pair<int, int>>> rle_vec = { rle_y, rle_cb, rle_cr };

	return make_tuple(info, str_vec, rle_vec);
}
