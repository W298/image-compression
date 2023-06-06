//
//	Dept. Software Convergence, Kyung Hee University
//	Prof. Daeho Lee, nize@khu.ac.kr
//
#include "KhuGleWin.h"
#include "KhuGleSignal.h"
#include "Colorspace.h"
#include "Compress.h"
#include "Decompress.h"
#include "SSIM.h"
#include <iostream>

#pragma warning(disable:4996)

#define _CRTDBG_MAP_ALLOC
#include <cstdlib>
#include <crtdbg.h>
#include <map>

#ifdef _DEBUG
#ifndef DBG_NEW
#define DBG_NEW new ( _NORMAL_BLOCK , __FILE__ , __LINE__ )
#define new DBG_NEW
#endif
#endif  // _DEBUG

#define LOAD		1
#define COMPRESS	2

void print_histogram(const std::map<int, int>& map, const int step = 5)
{
	int count = 0;
	int start = -100;
	int sum = 0;
	int pre = 0;
	for (std::pair<const int, int> pair : map)
	{
		if (count == step)
		{
			std::cout << start << "~" << pre << "\t";
			printf("\x1b[37;47m");
			for (int j = 0; j < max(1, min(sum / 30, 80)); j++)
			{
				std::cout << "#";
			}
			printf("\x1b[39;49m");
			std::cout << "  [" << sum << "]\n";
			count = 0;
			start = -100;
			sum = 0;
		}

		if (start == -100)
		{
			start = pair.first;
		}

		sum += pair.second;
		pre = pair.first;
		count++;
	}
}

class CKhuGleImageLayer : public CKhuGleLayer {
public:
	std::vector<CKhuGleSignal> m_ImageVec;
	// CKhuGleSignal m_Image, m_ImageOut;
	int m_OffsetX, m_OffsetY;

	CKhuGleImageLayer(int nW, int nH, KgColor24 bgColor, CKgPoint ptPos = CKgPoint(0, 0))
		: CKhuGleLayer(nW, nH, bgColor, ptPos), m_ImageVec(5, CKhuGleSignal()), m_OffsetX(50), m_OffsetY(50) {}
	void DrawBackgroundImage();
};

void CKhuGleImageLayer::DrawBackgroundImage()
{
	for(int y = 0 ; y < m_nH ; y++)
	{
		for (int x = 0; x < m_nW; x++)
		{
			m_ImageBgR[y][x] = KgGetRed(m_bgColor);
			m_ImageBgG[y][x] = KgGetGreen(m_bgColor);
			m_ImageBgB[y][x] = KgGetBlue(m_bgColor);
		}
	}

	int index = 1;
	for (const CKhuGleSignal& image : m_ImageVec)
	{
		if (image.m_Red && image.m_Green && image.m_Blue)
		{
			for (int y = 0; y < image.m_nH && y < m_nH; ++y)
			{
				for (int x = 0; x < image.m_nW && x < m_nW; ++x)
				{
					m_ImageBgR[y + m_OffsetY][x + m_OffsetX * index + image.m_nW * (index - 1)] = image.m_Red[y][x];
					m_ImageBgG[y + m_OffsetY][x + m_OffsetX * index + image.m_nW * (index - 1)] = image.m_Green[y][x];
					m_ImageBgB[y + m_OffsetY][x + m_OffsetX * index + image.m_nW * (index - 1)] = image.m_Blue[y][x];
				}
			}
		}

		index++;
	}
}

class CImageProcessing : public CKhuGleWin {
public:
	CKhuGleImageLayer *m_pImageLayer;
	int mode;
	char test_str[100];

	CImageProcessing(int nW, int nH, int mode, char* ImagePath, char* ExportPath);
	void Update();
	void Compress(const char* write_path);
	void Decompress(const char* read_path);
};

CImageProcessing::CImageProcessing(int nW, int nH, int mode, char* ImagePath, char* ExportPath)
	: CKhuGleWin(nW, nH) {
	m_pScene = new CKhuGleScene(1580, 520, KG_COLOR_24_RGB(45, 45, 45));
	m_pImageLayer = new CKhuGleImageLayer(1580, 440, KG_COLOR_24_RGB(83, 83, 83), CKgPoint(0, 50));

	this->mode = mode;

	if (mode == LOAD)
	{
		Decompress(ImagePath);
	}
	else if (mode == COMPRESS)
	{
		m_pImageLayer->m_ImageVec[0].ReadBmp(ImagePath);
		Compress(ExportPath);
	}

	m_pImageLayer->DrawBackgroundImage();
	m_pScene->AddChild(m_pImageLayer);
}

void CImageProcessing::Update()
{
	m_pScene->Render();
	DrawSceneTextPos("Image Compression", CKgPoint(30, 10), RGB(255, 255, 255), "Cascadia Code", FW_BOLD);
	DrawSceneTextPos("Original", CKgPoint(130, 380), RGB(255, 255, 255), "Cascadia Code", FW_BOLD);
	DrawSceneTextPos("DWT (Forward)", CKgPoint(410, 380), RGB(255, 255, 255), "Cascadia Code", FW_BOLD);
	DrawSceneTextPos("Quantization Step Size", CKgPoint(670, 380), RGB(255, 255, 255), "Cascadia Code", FW_BOLD);
	DrawSceneTextPos("Encoded Size", CKgPoint(1030, 380), RGB(255, 255, 255), "Cascadia Code", FW_BOLD);
	DrawSceneTextPos("Decompressed Image (Test)", CKgPoint(1270, 380), RGB(255, 255, 255), "Cascadia Code", FW_BOLD);
	DrawSceneTextPos(test_str, CKgPoint(50, 450), RGB(255, 255, 255), "Cascadia Code", FW_BOLD);
	
	CKhuGleWin::Update();
}

void CImageProcessing::Compress(const char* write_path)
{
	const int height = 256;
	const int width = 256;

	double** original_r = dmatrix(height, width);
	double** original_g = dmatrix(height, width);
	double** original_b = dmatrix(height, width);

	double** test_r = dmatrix(height, width);
	double** test_g = dmatrix(height, width);
	double** test_b = dmatrix(height, width);

	for (int y = 0; y < height; ++y)
	{
		for (int x = 0; x < width; ++x)
		{
			original_r[y][x] = m_pImageLayer->m_ImageVec[0].m_Red[y][x];
			original_g[y][x] = m_pImageLayer->m_ImageVec[0].m_Green[y][x];
			original_b[y][x] = m_pImageLayer->m_ImageVec[0].m_Blue[y][x];
		}
	}

	double** original_y = dmatrix(height, width);
	double** original_cb = dmatrix(height / 2, width / 2);
	double** original_cr = dmatrix(height / 2, width / 2);

	double** test_y = dmatrix(height, width);
	double** test_cb = dmatrix(height / 2, width / 2);
	double** test_cr = dmatrix(height / 2, width / 2);

	RGB2YCbCr(original_r, original_g, original_b, original_y, original_cb, original_cr, height, width);
	compress_image(write_path, original_y, original_cb, original_cr, height, width, m_pImageLayer->m_ImageVec);

	m_pImageLayer->m_ImageVec[4].m_Red = cmatrix(m_nH, m_nW);
	m_pImageLayer->m_ImageVec[4].m_Green = cmatrix(m_nH, m_nW);
	m_pImageLayer->m_ImageVec[4].m_Blue = cmatrix(m_nH, m_nW);

	m_pImageLayer->m_ImageVec[4].m_nH = height;
	m_pImageLayer->m_ImageVec[4].m_nW = width;

	decompress_image(write_path, test_y, test_cb, test_cr);
	YCbCr2RGB(test_y, test_cb, test_cr, test_r, test_g, test_b, height, width);

	for (int y = 0; y < height; ++y)
	{
		for (int x = 0; x < width; ++x)
		{
			m_pImageLayer->m_ImageVec[4].m_Red[y][x] = test_r[y][x];
			m_pImageLayer->m_ImageVec[4].m_Green[y][x] = test_g[y][x];
			m_pImageLayer->m_ImageVec[4].m_Blue[y][x] = test_b[y][x];
		}
	}

	double psnr = GetPsnr(
		m_pImageLayer->m_ImageVec[0].m_Red,
		m_pImageLayer->m_ImageVec[0].m_Green,
		m_pImageLayer->m_ImageVec[0].m_Blue,
		m_pImageLayer->m_ImageVec[4].m_Red,
		m_pImageLayer->m_ImageVec[4].m_Green,
		m_pImageLayer->m_ImageVec[4].m_Blue,
		width, height);

	double ssim = compute_ssim(
		m_pImageLayer->m_ImageVec[0].m_Red,
		m_pImageLayer->m_ImageVec[4].m_Red,
		width, height);

	sprintf(test_str, "PSNR : %.3f       SSIM : %.3f", psnr, ssim);

	free_dmatrix(original_r, height, width);
	free_dmatrix(original_g, height, width);
	free_dmatrix(original_b, height, width);

	free_dmatrix(test_r, height, width);
	free_dmatrix(test_g, height, width);
	free_dmatrix(test_b, height, width);

	free_dmatrix(original_y, height, width);
	free_dmatrix(original_cb, height / 2, width / 2);
	free_dmatrix(original_cr, height / 2, width / 2);

	free_dmatrix(test_y, height, width);
	free_dmatrix(test_cb, height / 2, width / 2);
	free_dmatrix(test_cr, height / 2, width / 2);

	m_pImageLayer->DrawBackgroundImage();
}

void CImageProcessing::Decompress(const char* read_path)
{
	const int height = 256;
	const int width = 256;

	double** decompressed_y = dmatrix(height, width);
	double** decompressed_cb = dmatrix(height / 2, width / 2);
	double** decompressed_cr = dmatrix(height / 2, width / 2);

	double** decompressed_r = dmatrix(height, width);
	double** decompressed_g = dmatrix(height, width);
	double** decompressed_b = dmatrix(height, width);

	decompress_image(read_path, decompressed_y, decompressed_cb, decompressed_cr);
	YCbCr2RGB(decompressed_y, decompressed_cb, decompressed_cr, decompressed_r, decompressed_g, decompressed_b, height, width);

	m_pImageLayer->m_ImageVec[0].m_Red = cmatrix(m_nH, m_nW);
	m_pImageLayer->m_ImageVec[0].m_Green = cmatrix(m_nH, m_nW);
	m_pImageLayer->m_ImageVec[0].m_Blue = cmatrix(m_nH, m_nW);

	m_pImageLayer->m_ImageVec[0].m_nH = height;
	m_pImageLayer->m_ImageVec[0].m_nW = width;

	for (int y = 0; y < height; y++)
	{
		for (int x = 0; x < width; x++)
		{
			m_pImageLayer->m_ImageVec[0].m_Red[y][x] = decompressed_r[y][x];
			m_pImageLayer->m_ImageVec[0].m_Green[y][x] = decompressed_g[y][x];
			m_pImageLayer->m_ImageVec[0].m_Blue[y][x] = decompressed_b[y][x];
		}
	}

	/*double psnr = GetPsnr(
		m_pImageLayer->m_Image.m_Red,
		m_pImageLayer->m_Image.m_Green,
		m_pImageLayer->m_Image.m_Blue,
		m_pImageLayer->m_ImageOut.m_Red,
		m_pImageLayer->m_ImageOut.m_Green,
		m_pImageLayer->m_ImageOut.m_Blue,
		m_pImageLayer->m_Image.m_nW, 
		m_pImageLayer->m_Image.m_nH);

	std::cout << "\n\n";
	std::cout << "PSNR : " << psnr << std::endl;

	double ssim = compute_ssim(
		m_pImageLayer->m_Image.m_Red,
		m_pImageLayer->m_ImageOut.m_Red,
		m_pImageLayer->m_Image.m_nW,
		m_pImageLayer->m_Image.m_nH);
	std::cout << "SSIM : " << ssim << "\n";*/

	free_dmatrix(decompressed_y, height, width);
	free_dmatrix(decompressed_cb, height / 2, width / 2);
	free_dmatrix(decompressed_cr, height / 2, width / 2);

	free_dmatrix(decompressed_r, height, width);
	free_dmatrix(decompressed_g, height, width);
	free_dmatrix(decompressed_b, height, width);

	m_pImageLayer->DrawBackgroundImage();
}

int main()
{
	char ExePath[MAX_PATH], ImagePath[MAX_PATH], ExportPath[MAX_PATH];

	GetModuleFileName(NULL, ExePath, MAX_PATH);

	int i;
	int LastBackSlash = -1;
	int nLen = strlen(ExePath);
	for(i = nLen-1 ; i >= 0 ; i--)
	{
		if(ExePath[i] == '\\') {
			LastBackSlash = i;
			break;
		}
	}

	if(LastBackSlash >= 0)
		ExePath[LastBackSlash] = '\0';

	int mode = 0;
	std::cout << "  _____ __  __          _____ ______    _____ ____  __  __ _____  _____  ______  _____ _____ _____ ____  _   _ " << "\n";
	std::cout << " |_   _|  \\/  |   /\\   / ____|  ____|  / ____/ __ \\|  \\/  |  __ \\|  __ \\|  ____|/ ____/ ____|_   _/ __ \\| \\ | |" << "\n";
	std::cout << "   | | | \\  / |  /  \\ | |  __| |__    | |   | |  | | \\  / | |__) | |__) | |__  | (___| (___   | || |  | |  \\| |" << "\n";
	std::cout << "   | | | |\\/| | / /\\ \\| | |_ |  __|   | |   | |  | | |\\/| |  ___/|  _  /|  __|  \\___ \\\\___ \\  | || |  | | . ` |" << "\n";
	std::cout << "  _| |_| |  | |/ ____ \\ |__| | |____  | |___| |__| | |  | | |    | | \\ \\| |____ ____) |___) |_| || |__| | |\\  |" << "\n";
	std::cout << " |_____|_|  |_/_/    \\_\\_____|______|  \\_____\\____/|_|  |_|_|    |_|  \\_\\______|_____/_____/|_____\\____/|_| \\_|" << "\n\n";
	std::cout << "\u001b[37;1m";
	std::cout << "================== Choose Mode ================" << "\n";
	std::cout << "=          1. Load Compressed Image          =" << "\n";
	std::cout << "=          2. Compress BMP Image             =" << "\n";
	std::cout << "================================================" << "\n\n";
	std::cout << "\u001b[0m";
	std::cout << "> ";
	std::cin >> mode;

	system("cls");

	char ImageName[100], ExportImageName[100];

	IMAGE_INPUT:
	if (mode == LOAD)
	{
		std::cout << "\u001b[32;1m[Load Compressed Image Mode Selected]\u001b[0m" << "\n\n";
		std::cout << "Enter Compressed Image Name (*.comp) : ";
	}
	else if (mode == COMPRESS)
	{
		std::cout << "\u001b[32;1m[Compress BMP Image Mode Selected]\u001b[0m" << "\n\n";
		std::cout << "Enter BMP Image Name (*.bmp) : ";
	}

	std::cin >> ImageName;
	sprintf(ImagePath, "%s\\%s", ExePath, ImageName);

	std::ifstream fin(ImagePath, std::ios::in | std::ios::binary);
	if (!fin)
	{
		system("cls");
		std::cout << "\u001b[41;1mFile Not Found. Try Again.\u001b[0m" << "\n";
		goto IMAGE_INPUT;
	}

	fin.close();

	EXPORT_INPUT:
	if (mode == COMPRESS)
	{
		std::cout << "Enter Export Image Name (*.comp) : ";
		std::cin >> ExportImageName;
		sprintf(ExportPath, "%s\\%s", ExePath, ExportImageName);
	}

	CImageProcessing *pImageProcessing = new CImageProcessing(1580, 520, mode, ImagePath, ExportPath);
	KhuGleWinInit(pImageProcessing);

	return 0;
}
