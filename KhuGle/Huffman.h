#pragma once

#include <vector>
#include <queue>
#include <map>
#include <string>

class HuffmanNode
{
public:
	int data_;
	unsigned freq_;
	HuffmanNode* left_;
	HuffmanNode* right_;

	HuffmanNode(int data, unsigned freq, HuffmanNode* left = nullptr, HuffmanNode* right = nullptr)
	{
		this->data_ = data;
		this->freq_ = freq;
		this->left_ = left;
		this->right_ = right;
	}
};

struct Compare
{
	bool operator()(HuffmanNode* l, HuffmanNode* r) const
	{
		return (l->freq_ > r->freq_);
	}
};

inline std::vector<std::vector<int>> DecodeWithHuffman(std::string& encoded_str, HuffmanNode* root, int originalSize)
{
	std::vector<std::vector<int>> originalData(originalSize, std::vector<int>(originalSize));

	int index = -1;
	HuffmanNode* curr = root;
	for (int i = 0; i < encoded_str.size(); i++)
	{
		if (encoded_str[i] == '0')
			curr = curr->left_;
		else
			curr = curr->right_;

		if (!curr->left_ && !curr->right_)
		{
			originalData[++index / originalSize][index % originalSize] = curr->data_;
			curr = root;
			continue;
		}
	}

	return originalData;
}

inline void EncodeLoop(HuffmanNode* root, std::string str, std::map<int, std::string>& huffman_code)
{
	if (root == nullptr) return;

	if (!root->left_ && !root->right_)
	{
		huffman_code[root->data_] = str;
	}

	EncodeLoop(root->left_, str + "0", huffman_code);
	EncodeLoop(root->right_, str + "1", huffman_code);
}

inline std::string EncodeWithHuffman(HuffmanNode* root, std::vector<std::vector<int>>& data)
{
	std::map<int, std::string> huffman_code;
	EncodeLoop(root, "", huffman_code);

	std::string str = "";
	for (int i = 0; i < data.size(); i++)
	{
		for (int j = 0; j < data[0].size(); j++)
		{
			str += huffman_code[data[i][j]];
		}
	}

	return str;
}

inline HuffmanNode* BuildHuffmanTree(std::vector<std::pair<int, int>>& rle)
{
	std::priority_queue<HuffmanNode*, std::vector<HuffmanNode*>, Compare> minHeap;

	for (auto& pair : rle)
	{
		minHeap.push(new HuffmanNode(pair.second, pair.first));
	}

	while (minHeap.size() != 1)
	{
		HuffmanNode* right = minHeap.top();
		minHeap.pop();
		HuffmanNode* left = minHeap.top();
		minHeap.pop();

		int sum = left->freq_ + right->freq_;
		minHeap.push(new HuffmanNode('\0', sum, left, right));
	}

	return minHeap.top();
}

inline std::vector<std::pair<int, int>> RunLengthEncoding(std::vector<std::vector<int>>& data)
{
	std::vector<std::pair<int, int>> rle;
	for (int i = 0; i < data.size(); i++)
	{
		for (int j = 0; j < data[0].size(); j++)
		{
			int index = -1;
			for (int k = 0; k < rle.size(); k++)
			{
				if (rle[k].second == data[i][j])
				{
					index = k;
					break;
				}
			}

			if (index == -1)
			{
				rle.emplace_back(1, data[i][j]);
			}
			else
			{
				rle[index].first++;
			}
		}
	}

	return rle;
}
