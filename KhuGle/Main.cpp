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

class CKhuGleImageLayer : public CKhuGleLayer
{
public:
	std::vector<CKhuGleSignal> m_ImageVec;
	int m_OffsetX, m_OffsetY;

	CKhuGleImageLayer(int nW, int nH, KgColor24 bgColor, CKgPoint ptPos = CKgPoint(0, 0))
		: CKhuGleLayer(nW, nH, bgColor, ptPos), m_ImageVec(5, CKhuGleSignal()), m_OffsetX(50), m_OffsetY(50)
	{
	}

	void DrawBackgroundImage() const;
};

void CKhuGleImageLayer::DrawBackgroundImage() const
{
	for (int y = 0; y < m_nH; y++)
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

class CImageProcessing : public CKhuGleWin
{
public:
	CKhuGleImageLayer* m_pImageLayer;
	std::string m_HeaderStr, m_ValueStr;
	std::string m_ResultStr, m_ResultStr2;
	CompResult* m_TempResult = nullptr;
	int m_CurrentMode = -1;

	CImageProcessing(int nW, int nH);
	void Update() override;
	void CleanUp();
	void LoadBMPAndCompress(const char* path, bool deep = false);
	void LoadComp(const char* read_path);
	void SaveAsComp(const char* write_path) const;
	void SaveAsBMP(const char* write_path) const;
	void OnFileEvent(std::string path, int mode) override;
};

CImageProcessing::CImageProcessing(int nW, int nH)
	: CKhuGleWin(nW, nH)
{
	m_pScene = new CKhuGleScene(1580, 580, KG_COLOR_24_RGB(210, 210, 210));
	m_pImageLayer = new CKhuGleImageLayer(1580, 580, KG_COLOR_24_RGB(210, 210, 210), CKgPoint(0, 30));

	m_pImageLayer->DrawBackgroundImage();
	m_pScene->AddChild(m_pImageLayer);
}

void CImageProcessing::Update()
{
	m_pScene->Render();

	if (m_CurrentMode == ID_FILE_LOAD_COMP)
	{
		DrawSceneTextPos("Decompression process", CKgPoint(50, 25), RGB(0, 0, 0), "Cascadia Code", FW_BOLD, 30);
		DrawSceneTextPos("Encoded Size", CKgPoint(110, 350), RGB(0, 0, 0), "Cascadia Code");
		DrawSceneTextPos("Quantization Step Size", CKgPoint(365, 350), RGB(0, 0, 0), "Cascadia Code");
		DrawSceneTextPos("DWT (Inverse)", CKgPoint(720, 350), RGB(0, 0, 0), "Cascadia Code");
		DrawSceneTextPos("Decompressed Image", CKgPoint(1000, 350), RGB(0, 0, 0), "Cascadia Code");
		DrawSceneTextPos("Result", CKgPoint(50, 430), RGB(0, 0, 0), "Cascadia Code", FW_BOLD, 30);
		DrawSceneTextPos(m_ResultStr.c_str(), CKgPoint(300, 480), RGB(0, 0, 0), "Cascadia Code");
		DrawSceneTextPos(m_ResultStr2.c_str(), CKgPoint(300, 510), RGB(0, 0, 0), "Cascadia Code");
		DrawSceneTextPos(m_HeaderStr.c_str(), CKgPoint(50, 480), RGB(0, 0, 0), "Cascadia Code");
		DrawSceneTextPos(m_ValueStr.c_str(), CKgPoint(50, 510), RGB(0, 0, 0), "Cascadia Code", FW_BOLD, 30);
	}
	else if (m_CurrentMode == ID_FILE_LOAD_BMP || m_CurrentMode == ID_FILE_LOAD_BMP_H)
	{
		DrawSceneTextPos(
			m_CurrentMode == ID_FILE_LOAD_BMP
				? "Compression process (Normal Compression Level)"
				: "Compression process (High Compression Level)", CKgPoint(50, 25), RGB(0, 0, 0), "Cascadia Code",
			FW_BOLD, 30);
		DrawSceneTextPos("Original", CKgPoint(130, 350), RGB(0, 0, 0), "Cascadia Code");
		DrawSceneTextPos("DWT (Forward)", CKgPoint(415, 350), RGB(0, 0, 0), "Cascadia Code");
		DrawSceneTextPos("Quantization Step Size", CKgPoint(670, 350), RGB(0, 0, 0), "Cascadia Code");
		DrawSceneTextPos("Encoded Size", CKgPoint(1030, 350), RGB(0, 0, 0), "Cascadia Code");
		DrawSceneTextPos("Decompressed Image", CKgPoint(1300, 350), RGB(0, 0, 0), "Cascadia Code");
		DrawSceneTextPos("Result", CKgPoint(50, 430), RGB(0, 0, 0), "Cascadia Code", FW_BOLD, 30);
		DrawSceneTextPos(m_ResultStr.c_str(), CKgPoint(800, 480), RGB(0, 0, 0), "Cascadia Code");
		DrawSceneTextPos(m_ResultStr2.c_str(), CKgPoint(800, 510), RGB(0, 0, 0), "Cascadia Code");
		DrawSceneTextPos(m_HeaderStr.c_str(), CKgPoint(50, 480), RGB(0, 0, 0), "Cascadia Code");
		DrawSceneTextPos(m_ValueStr.c_str(), CKgPoint(50, 510), RGB(0, 0, 0), "Cascadia Code", FW_BOLD, 30);
	}

	CKhuGleWin::Update();
}

void CImageProcessing::CleanUp()
{
	m_TempResult = nullptr;
	m_ResultStr = "";
	m_ResultStr2 = "";
	m_HeaderStr = "";
	m_ValueStr = "";

	delete m_TempResult;
}

void CImageProcessing::LoadBMPAndCompress(const char* path, bool deep)
{
	int image_size[2];
	m_pImageLayer->m_ImageVec[0].ReadBmp(path, image_size);

	const int bmp_height = image_size[0];
	const int bmp_width = image_size[1];

	double** original_r = dmatrix(bmp_height, bmp_width);
	double** original_g = dmatrix(bmp_height, bmp_width);
	double** original_b = dmatrix(bmp_height, bmp_width);

	double** original_y = dmatrix(bmp_height, bmp_width);
	double** original_cb = dmatrix(bmp_height / 2, bmp_width / 2);
	double** original_cr = dmatrix(bmp_height / 2, bmp_width / 2);

	for (int y = 0; y < bmp_height; ++y)
	{
		for (int x = 0; x < bmp_width; ++x)
		{
			original_r[y][x] = m_pImageLayer->m_ImageVec[0].m_Red[y][x];
			original_g[y][x] = m_pImageLayer->m_ImageVec[0].m_Green[y][x];
			original_b[y][x] = m_pImageLayer->m_ImageVec[0].m_Blue[y][x];
		}
	}

	RGB2YCbCr(original_r, original_g, original_b, original_y, original_cb, original_cr, bmp_height, bmp_width);
	CompResult* result = CompressImage(original_y, original_cb, original_cr, bmp_height, bmp_width, deep,
	                                   m_pImageLayer->m_ImageVec, m_ResultStr, m_ResultStr2);

	m_TempResult = result;

	m_pImageLayer->m_ImageVec[4].m_Red = cmatrix(m_nH, m_nW);
	m_pImageLayer->m_ImageVec[4].m_Green = cmatrix(m_nH, m_nW);
	m_pImageLayer->m_ImageVec[4].m_Blue = cmatrix(m_nH, m_nW);

	m_pImageLayer->m_ImageVec[4].m_nH = bmp_height;
	m_pImageLayer->m_ImageVec[4].m_nW = bmp_width;

	double** test_r = dmatrix(bmp_height, bmp_width);
	double** test_g = dmatrix(bmp_height, bmp_width);
	double** test_b = dmatrix(bmp_height, bmp_width);

	double** test_y = dmatrix(bmp_height, bmp_width);
	double** test_cb = dmatrix(bmp_height / 2, bmp_width / 2);
	double** test_cr = dmatrix(bmp_height / 2, bmp_width / 2);

	DecompressImage(result, test_y, test_cb, test_cr);
	YCbCr2RGB(test_y, test_cb, test_cr, test_r, test_g, test_b, bmp_height, bmp_width);

	const int original_file_size = MeasureFileSize(path);
	const int file_size = MeasureFileSize(result);

	for (int y = 0; y < bmp_height; ++y)
	{
		for (int x = 0; x < bmp_width; ++x)
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
		bmp_width, bmp_height);

	double ssim = ComputeSSIM(
		m_pImageLayer->m_ImageVec[0].m_Red,
		m_pImageLayer->m_ImageVec[4].m_Red,
		bmp_width, bmp_height);

	m_HeaderStr = "PSNR         SSIM        FILE SIZE";

	char buffer[100];
	sprintf(buffer, "%6.3f     %4.3f     %6.3fKB (%.3f %% of Original)", psnr, ssim, (double)file_size / 1024,
	        (double)file_size / (double)original_file_size * 100);
	m_ValueStr = std::string(buffer);

	free_dmatrix(original_r, bmp_height, bmp_width);
	free_dmatrix(original_g, bmp_height, bmp_width);
	free_dmatrix(original_b, bmp_height, bmp_width);

	free_dmatrix(test_r, bmp_height, bmp_width);
	free_dmatrix(test_g, bmp_height, bmp_width);
	free_dmatrix(test_b, bmp_height, bmp_width);

	free_dmatrix(original_y, bmp_height, bmp_width);
	free_dmatrix(original_cb, bmp_height / 2, bmp_width / 2);
	free_dmatrix(original_cr, bmp_height / 2, bmp_width / 2);

	free_dmatrix(test_y, bmp_height, bmp_width);
	free_dmatrix(test_cb, bmp_height / 2, bmp_width / 2);
	free_dmatrix(test_cr, bmp_height / 2, bmp_width / 2);

	m_pImageLayer->DrawBackgroundImage();
}

void CImageProcessing::LoadComp(const char* read_path)
{
	const std::pair<int, int> image_size = MeasureImageSize(read_path);
	const int comp_height = image_size.first;
	const int comp_width = image_size.second;

	double** decompressed_y = dmatrix(comp_height, comp_width);
	double** decompressed_cb = dmatrix(comp_height / 2, comp_width / 2);
	double** decompressed_cr = dmatrix(comp_height / 2, comp_width / 2);

	double** decompressed_r = dmatrix(comp_height, comp_width);
	double** decompressed_g = dmatrix(comp_height, comp_width);
	double** decompressed_b = dmatrix(comp_height, comp_width);

	DecompressImage(read_path, decompressed_y, decompressed_cb, decompressed_cr, m_pImageLayer->m_ImageVec, m_ResultStr,
	                m_ResultStr2);
	YCbCr2RGB(decompressed_y, decompressed_cb, decompressed_cr, decompressed_r, decompressed_g, decompressed_b,
	          comp_height, comp_width);

	m_pImageLayer->m_ImageVec[3].m_Red = cmatrix(m_nH, m_nW);
	m_pImageLayer->m_ImageVec[3].m_Green = cmatrix(m_nH, m_nW);
	m_pImageLayer->m_ImageVec[3].m_Blue = cmatrix(m_nH, m_nW);

	m_pImageLayer->m_ImageVec[3].m_nH = comp_height;
	m_pImageLayer->m_ImageVec[3].m_nW = comp_width;

	for (int y = 0; y < comp_height; y++)
	{
		for (int x = 0; x < comp_width; x++)
		{
			m_pImageLayer->m_ImageVec[3].m_Red[y][x] = decompressed_r[y][x];
			m_pImageLayer->m_ImageVec[3].m_Green[y][x] = decompressed_g[y][x];
			m_pImageLayer->m_ImageVec[3].m_Blue[y][x] = decompressed_b[y][x];
		}
	}

	m_HeaderStr = "FILE SIZE";

	const int file_size = MeasureFileSize(read_path);

	char buffer[100];
	sprintf(buffer, "%6.3fKB", (double)file_size / 1024);
	m_ValueStr = std::string(buffer);

	free_dmatrix(decompressed_y, comp_height, comp_width);
	free_dmatrix(decompressed_cb, comp_height / 2, comp_width / 2);
	free_dmatrix(decompressed_cr, comp_height / 2, comp_width / 2);

	free_dmatrix(decompressed_r, comp_height, comp_width);
	free_dmatrix(decompressed_g, comp_height, comp_width);
	free_dmatrix(decompressed_b, comp_height, comp_width);

	m_pImageLayer->DrawBackgroundImage();
}

void CImageProcessing::SaveAsComp(const char* write_path) const
{
	if (m_TempResult == nullptr)
	{
		MessageBox(m_hWnd, "Please Load BMP Image First!", "Alert", MB_OK | MB_ICONWARNING);
		return;
	}

	WriteAll(write_path, m_TempResult);
}

void CImageProcessing::SaveAsBMP(const char* write_path) const
{
	if (m_CurrentMode != ID_FILE_LOAD_COMP)
	{
		MessageBox(m_hWnd, "Please Load Compressed Image First!", "Alert", MB_OK | MB_ICONWARNING);
		return;
	}

	m_pImageLayer->m_ImageVec[3].SaveBmp(write_path);
}

void CImageProcessing::OnFileEvent(std::string path, int mode)
{
	if (path.empty()) return;

	if (mode != ID_FILE_SAVE_COMP && mode != ID_FILE_SAVE_BMP)
		m_CurrentMode = mode;

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
	case ID_FILE_LOAD_BMP_H:
		CleanUp();
		LoadBMPAndCompress(path.c_str(), true);
		break;
	case ID_FILE_SAVE_COMP:
		SaveAsComp(path.c_str());
		break;
	case ID_FILE_SAVE_BMP:
		SaveAsBMP(path.c_str());
		break;
	}
}

int main()
{
	CImageProcessing* pImageProcessing = new CImageProcessing(1580, 580);
	KhuGleWinInit(pImageProcessing);

	return 0;
}
