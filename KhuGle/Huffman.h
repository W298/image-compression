#pragma once

#include <vector>
#include <queue>
#include <map>
#include <string>

class HuffmanNode
{
public:
	int data;
	unsigned freq;
	HuffmanNode* left;
	HuffmanNode* right;

	HuffmanNode(int data, unsigned freq, HuffmanNode* left = nullptr, HuffmanNode* right = nullptr)
	{
		this->data = data;
		this->freq = freq;
		this->left = left;
		this->right = right;
	}
};

struct compare
{
	bool operator()(HuffmanNode* l, HuffmanNode* r)
	{
		return (l->freq > r->freq);
	}
};

inline std::vector<std::vector<int>> decode_with_huffman(std::string& encoded_str, HuffmanNode* root, int originalSize)
{
	std::vector<std::vector<int>> originalData(originalSize, std::vector<int>(originalSize));

	int index = -1;
	HuffmanNode* curr = root;
	for (int i = 0; i < encoded_str.size(); i++)
	{
		if (encoded_str[i] == '0')
			curr = curr->left;
		else
			curr = curr->right;

		if (!curr->left && !curr->right)
		{
			originalData[++index / originalSize][index % originalSize] = curr->data;
			curr = root;
			continue;
		}
	}

	return originalData;
}

inline void encode_loop(HuffmanNode* root, std::string str, std::map<int, std::string>& huffman_code)
{
	if (root == nullptr) return;

	if (!root->left && !root->right)
	{
		huffman_code[root->data] = str;
	}

	encode_loop(root->left, str + "0", huffman_code);
	encode_loop(root->right, str + "1", huffman_code);
}

inline std::string encode_with_huffman(HuffmanNode* root, std::vector<std::vector<int>>& data)
{
	std::map<int, std::string> huffman_code;
	encode_loop(root, "", huffman_code);

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

inline HuffmanNode* build_huffman_tree(std::vector<std::pair<int, int>>& rle)
{
	std::priority_queue<HuffmanNode*, std::vector<HuffmanNode*>, compare> minHeap;

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

		int sum = left->freq + right->freq;
		minHeap.push(new HuffmanNode('\0', sum, left, right));
	}

	return minHeap.top();
}

inline std::vector<std::pair<int, int>> run_length_encoding(std::vector<std::vector<int>>& data)
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
