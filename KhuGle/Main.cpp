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
#include "Type.h"
#include <iostream>

#pragma warning(disable:4996)

#define _CRTDBG_MAP_ALLOC
#include <cstdlib>
#include <crtdbg.h>

#ifdef _DEBUG
#ifndef DBG_NEW
#define DBG_NEW new ( _NORMAL_BLOCK , __FILE__ , __LINE__ )
#define new DBG_NEW
#endif
#endif  // _DEBUG

class CKhuGleImageLayer : public CKhuGleLayer {
public:
	std::vector<CKhuGleSignal> m_ImageVec;
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
	CKhuGleImageLayer* m_pImageLayer;
	std::string m_ResultStr;
	CompResult* m_TempResult = nullptr;

	CImageProcessing(int nW, int nH);
	void Update();
	void CleanUp();
	void LoadBMPAndCompress(const char* path);
	void LoadComp(const char* read_path);
	void SaveAsComp(const char* write_path);
	void SaveAsBMP();
	void OnFileEvent(std::string path, int mode) override;
};

CImageProcessing::CImageProcessing(int nW, int nH)
	: CKhuGleWin(nW, nH) {
	m_pScene = new CKhuGleScene(1580, 550, KG_COLOR_24_RGB(210, 210, 210));
	m_pImageLayer = new CKhuGleImageLayer(1580, 550, KG_COLOR_24_RGB(210, 210, 210), CKgPoint(0, 30));

	m_pImageLayer->DrawBackgroundImage();
	m_pScene->AddChild(m_pImageLayer);
}

void CImageProcessing::Update()
{
	m_pScene->Render();
	DrawSceneTextPos("Compression process", CKgPoint(50, 25), RGB(0, 0, 0), "Cascadia Code", FW_BOLD, 30);
	DrawSceneTextPos("Original", CKgPoint(130, 350), RGB(0, 0, 0), "Cascadia Code");
	DrawSceneTextPos("DWT (Forward)", CKgPoint(410, 350), RGB(0, 0, 0), "Cascadia Code");
	DrawSceneTextPos("Quantization Step Size", CKgPoint(670, 350), RGB(0, 0, 0), "Cascadia Code");
	DrawSceneTextPos("Encoded Size", CKgPoint(1030, 350), RGB(0, 0, 0), "Cascadia Code");
	DrawSceneTextPos("Decompressed Image", CKgPoint(1300, 350), RGB(0, 0, 0), "Cascadia Code");
	DrawSceneTextPos("Result", CKgPoint(50, 430), RGB(0, 0, 0), "Cascadia Code", FW_BOLD, 30);
	DrawSceneTextPos(m_ResultStr.c_str(), CKgPoint(50, 480), RGB(0, 0, 0), "Cascadia Code");
	
	CKhuGleWin::Update();
}

void CImageProcessing::CleanUp()
{
	m_TempResult = nullptr;
	m_ResultStr = "";

	for (int i = 0; i < 5; i++)
	{
		m_pImageLayer->m_ImageVec[i] = CKhuGleSignal();
	}
}

void CImageProcessing::LoadBMPAndCompress(const char* path)
{
	m_pImageLayer->m_ImageVec[0].ReadBmp(path);

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
	CompResult* result = compress_image(original_y, original_cb, original_cr, height, width, m_pImageLayer->m_ImageVec);

	m_TempResult = result;

	m_pImageLayer->m_ImageVec[4].m_Red = cmatrix(m_nH, m_nW);
	m_pImageLayer->m_ImageVec[4].m_Green = cmatrix(m_nH, m_nW);
	m_pImageLayer->m_ImageVec[4].m_Blue = cmatrix(m_nH, m_nW);

	m_pImageLayer->m_ImageVec[4].m_nH = height;
	m_pImageLayer->m_ImageVec[4].m_nW = width;

	decompress_image(result, test_y, test_cb, test_cr);
	YCbCr2RGB(test_y, test_cb, test_cr, test_r, test_g, test_b, height, width);

	const int original_file_size = measure_file_size(path);
	const int file_size = measure_file_size(result);

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

	char buffer[100];
	sprintf(buffer, "PSNR : %.3f        SSIM : %.3f        File Size : %.3f KB (%.3f %% of Original Image)", 
		psnr, ssim, (double)file_size / 1024, (double)file_size / (double)original_file_size * 100
	);
	m_ResultStr = std::string(buffer);

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

void CImageProcessing::LoadComp(const char* read_path)
{
	const int height = 256;
	const int width = 256;

	double** decompressed_y = dmatrix(height, width);
	double** decompressed_cb = dmatrix(height / 2, width / 2);
	double** decompressed_cr = dmatrix(height / 2, width / 2);

	double** decompressed_r = dmatrix(height, width);
	double** decompressed_g = dmatrix(height, width);
	double** decompressed_b = dmatrix(height, width);

	decompress_image(read_path, decompressed_y, decompressed_cb, decompressed_cr, m_pImageLayer->m_ImageVec);
	YCbCr2RGB(decompressed_y, decompressed_cb, decompressed_cr, decompressed_r, decompressed_g, decompressed_b, height, width);

	m_pImageLayer->m_ImageVec[3].m_Red = cmatrix(m_nH, m_nW);
	m_pImageLayer->m_ImageVec[3].m_Green = cmatrix(m_nH, m_nW);
	m_pImageLayer->m_ImageVec[3].m_Blue = cmatrix(m_nH, m_nW);

	m_pImageLayer->m_ImageVec[3].m_nH = height;
	m_pImageLayer->m_ImageVec[3].m_nW = width;

	for (int y = 0; y < height; y++)
	{
		for (int x = 0; x < width; x++)
		{
			m_pImageLayer->m_ImageVec[3].m_Red[y][x] = decompressed_r[y][x];
			m_pImageLayer->m_ImageVec[3].m_Green[y][x] = decompressed_g[y][x];
			m_pImageLayer->m_ImageVec[3].m_Blue[y][x] = decompressed_b[y][x];
		}
	}

	free_dmatrix(decompressed_y, height, width);
	free_dmatrix(decompressed_cb, height / 2, width / 2);
	free_dmatrix(decompressed_cr, height / 2, width / 2);

	free_dmatrix(decompressed_r, height, width);
	free_dmatrix(decompressed_g, height, width);
	free_dmatrix(decompressed_b, height, width);

	m_pImageLayer->DrawBackgroundImage();
}

void CImageProcessing::SaveAsComp(const char* write_path)
{
	if (m_TempResult == nullptr)
	{
		MessageBox(m_hWnd, "Please Load BMP Image First!", "Alert", MB_OK | MB_ICONWARNING);
		return;
	}

	write_all(write_path, m_TempResult);
}

void CImageProcessing::OnFileEvent(std::string path, int mode)
{
	if (path.empty()) return;

	switch (mode)
	{
	case ID_FILE_LOAD_COMP:
		CleanUp();
		LoadComp(path.c_str());
		break;
	case ID_FILE_LOAD_BMP:
		CleanUp();
		LoadBMPAndCompress(path.c_str());
		break;
	case ID_FILE_SAVE_COMP:
		SaveAsComp(path.c_str());
		break;
	}
}

int main()
{
	CImageProcessing* pImageProcessing = new CImageProcessing(1580, 550);
	KhuGleWinInit(pImageProcessing);

	return 0;
}
