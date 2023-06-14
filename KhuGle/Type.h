#pragma once

struct HeaderInfo
{
	int y_len, cb_len, cr_len;
	int rle_y_len, rle_cb_len, rle_cr_len;
	int h, w;
	int y_pad, cb_pad, cr_pad;
	bool deep;

	HeaderInfo() : y_len(0), cb_len(0),
	               cr_len(0), rle_y_len(0),
	               rle_cb_len(0),
	               rle_cr_len(0), h(0), w(0), y_pad(0), cb_pad(0), cr_pad(0), deep(0)
	{
	}

	HeaderInfo(int y_len, int cb_len, int cr_len, int rle_y_len, int rle_cb_len, int rle_cr_len, int w, int h,
	           int y_pad, int cb_pad, int cr_pad, bool deep) : y_len(y_len),
	                                                           cb_len(cb_len),
	                                                           cr_len(cr_len), rle_y_len(rle_y_len),
	                                                           rle_cb_len(rle_cb_len),
	                                                           rle_cr_len(rle_cr_len), h(h), w(w), y_pad(y_pad),
	                                                           cb_pad(cb_pad),
	                                                           cr_pad(cr_pad), deep(deep)
	{
	}
};

class CompResult
{
public:
	HeaderInfo info_;

	std::string encoded_data_y_;
	std::string encoded_data_cb_;
	std::string encoded_data_cr_;

	std::vector<std::pair<int, int>> rle_y_;
	std::vector<std::pair<int, int>> rle_cb_;
	std::vector<std::pair<int, int>> rle_cr_;
};
