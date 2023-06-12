#pragma once

#include <windows.h>
#include <commdlg.h>
#include <string>

inline std::string FileLoad(HWND hwnd, TCHAR filter[])
{
	OPENFILENAME OFN;
	TCHAR lpstrFile[100] = "";

	memset(&OFN, 0, sizeof(OPENFILENAME));
	OFN.lStructSize = sizeof(OPENFILENAME);
	OFN.hwndOwner = hwnd;
	OFN.lpstrFilter = filter;
	OFN.lpstrFile = lpstrFile;
	OFN.nMaxFile = 100;
	OFN.lpstrInitialDir = ".";

	if (GetOpenFileName(&OFN) != 0)
	{
		std::string path = OFN.lpstrFile;
		return path;
	}

	return "";
}

inline std::string FileSave(HWND hwnd, TCHAR filter[])
{
	OPENFILENAME OFN;
	TCHAR lpstrFile[100] = "";

	memset(&OFN, 0, sizeof(OPENFILENAME));
	OFN.lStructSize = sizeof(OPENFILENAME);
	OFN.hwndOwner = hwnd;
	OFN.lpstrFilter = filter;
	OFN.lpstrFile = lpstrFile;
	OFN.nMaxFile = 100;
	OFN.lpstrInitialDir = ".";

	if (GetSaveFileName(&OFN) != 0)
	{
		std::string path = OFN.lpstrFile;
		return path;
	}

	return "";
}
