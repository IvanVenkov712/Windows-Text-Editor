/** @file FileHandling.cpp
 * @brief Unsaved-change gating and whole-document encoding conversions/I/O.
 * @details Public contracts and current error-handling limits are in
 * ProgramHeader.h; all routines operate on the shared UI/document state.
 */
#include "pch.h"
#include "ProgramHeader.h"
LRESULT ExecuteHandleUnsaved(PHANDLEUNSAVEDFUNC Func, LPARAM lParam)
{
	LRESULT toRet = 0;
	if (haveUnsaved)
	{
		HRESULT res = MessageBox(hWndMain,
			(L"You have unsaved changes. Do you want to save them?"),
			(L"Save"), MB_ICONQUESTION | MB_YESNOCANCEL);
		switch (res)
		{
		case IDYES:
			{
				if (fileName == nullptr)
				{
					if(FAILED(ShowSaveFileDialog())) return 0;
				}
				else SaveWorkingFile();
				toRet = Func(lParam);
				break;
			}
		case IDNO:
			toRet = Func(lParam);
			break;
		default:
			return 0;
		}
		haveUnsaved = false;
	}
	else toRet = Func(lParam);
	return toRet;
}
LRESULT  OpenWorkingFile()
{
	std::ifstream stream(fileName, std::ios::binary);
	UINT64 fileSize = std::filesystem::file_size(fileName);
	char* buff = new char[fileSize + 4];
	stream.read(buff, fileSize);
	stream.close();
	ZeroMemory(buff + fileSize, 4);

	if (encoding == ENCODING_AUTO_FIND)
	{
		//Try to detect the encoding of the file
		UWORD BOM_16 = *(reinterpret_cast<UWORD*>(buff));
		UINT32 BOM_8 = *(reinterpret_cast<UINT32*>(buff)) & 0x00FFFFFF;
		INT test1 = IS_TEXT_UNICODE_ASCII16;
		INT test2 = IS_TEXT_UNICODE_REVERSE_ASCII16;
		if (BOM_16 == 0xFEFF) encoding = ENCODING_UTF16_LE;
		else if (BOM_16 == 0xFFFE) encoding = ENCODING_UTF16_BE;
		else if (BOM_8 == 0xBFBBEF) encoding = ENCODING_UTF8_BOM;
		else if (IsTextUnicode(buff, fileSize, &test1)) encoding = ENCODING_UTF16_LE;
		else if (IsTextUnicode(buff, fileSize, &test2)) encoding = ENCODING_UTF16_BE;
		else encoding = ENCODING_UTF8;
	}
	switch (encoding)
	{
	case ENCODING_UTF8:
		{
			//Open as "UTF-8" encoded text file
			LoadBufferUTF_8(buff);
			break;
		}
	case ENCODING_UTF16_LE:
		{
			//Open as "UTF-16 Little Endian" encoded text file
			LoadBufferUTF_16_LE(buff);
			break;
		}
	case ENCODING_UTF16_BE:
		{
			//Open as "UTF-16 Big Endian" encoded text file
			LoadBufferUTF_16_BE(buff, fileSize);
			break;
		}
	case ENCODING_UTF8_BOM:
		{
			//Open as "UTF-8" encoded text file with BOM
			LoadBufferUTF_8(buff + 3);
			SendMessageW(hStatus, SB_SETTEXTW, MAKEWORD(4, 0), reinterpret_cast<LPARAM>(L"UTF-8 BOM"));
			break;
		}
	}
	SetWindowTextW(hDirEdit, fileName);
	EnableMenuItem(GetMenu(hWndMain), ID_FILE_SAVE, MF_BYCOMMAND | MF_ENABLED);
	delete[] buff;

	EC_ENDOFLINE lineEnd = Edit_GetEndOfLine(hMainEdit);
	switch (lineEnd)
	{
	case EC_ENDOFLINE_CRLF:
		SendMessageW(hStatus, SB_SETTEXTW, MAKEWORD(3, 0), reinterpret_cast<LPARAM>(L"Windows (CRLF)"));
		break;
	case EC_ENDOFLINE_CR:
		SendMessageW(hStatus, SB_SETTEXTW, MAKEWORD(3, 0), reinterpret_cast<LPARAM>(L"Macintosh (CR)"));
		break;
	case EC_ENDOFLINE_LF:
		SendMessageW(hStatus, SB_SETTEXTW, MAKEWORD(3, 0), reinterpret_cast<LPARAM>(L"Unix (LF)"));
		break;
	}
	return encoding;
}

void LoadBufferUTF_8(char* buff)
{
	int count = MultiByteToWideChar(CP_UTF8, 0, buff, -1, nullptr, 0);
	LPWSTR wsBuff = new WCHAR[count];
	MultiByteToWideChar(CP_UTF8, 0, buff, -1, wsBuff, count);
	count = NormalizeString(NormalizationC, wsBuff, -1, nullptr, 0);
	LPWSTR wsNormalBuff = new WCHAR[count];
	NormalizeString(NormalizationC, wsBuff, -1, wsNormalBuff, count);
	SetWindowTextW(hMainEdit, wsNormalBuff);
	delete[] wsBuff;
	delete[] wsNormalBuff;
	SendMessageW(hStatus, SB_SETTEXTW, MAKEWORD(4, 0), reinterpret_cast<LPARAM>(L"UTF-8"));
}

void LoadBufferUTF_16_LE(char* buff)
{
	LPWSTR wsBuff = reinterpret_cast<LPWSTR>(buff);
	if (wsBuff[0] == 0xFFFE || wsBuff[0] == 0xFEFF)
	{
		SetWindowTextW(hMainEdit, wsBuff + 1);
	}
	else SetWindowTextW(hMainEdit, wsBuff);
	SendMessageW(hStatus, SB_SETTEXTW, MAKEWORD(4, 0), reinterpret_cast<LPARAM>(L"UTF-16 LE"));
}

void LoadBufferUTF_16_BE(char* buff, UINT64 fileSize)
{
	LPWSTR wsBuff = reinterpret_cast<LPWSTR>(buff);

	//Byte swap is necessary because Windows resolves only Little-Endian
	for (uint16_t i = 0; i < fileSize / sizeof(WCHAR); i++)
	{
		wsBuff[i] = _byteswap_ushort(wsBuff[i]);
	}
	if (wsBuff[0] == 0xFFFE || wsBuff[0] == 0xFEFF)
	{
		SetWindowTextW(hMainEdit, wsBuff + 1);
	}
	else SetWindowTextW(hMainEdit, wsBuff);
	SendMessageW(hStatus, SB_SETTEXTW, MAKEWORD(4, 0), reinterpret_cast<LPARAM>(L"UTF-16 BE"));
}

void SaveWorkingFile()
{
	std::ofstream stream(fileName, std::ios::binary | std::ios::trunc);
	int lng = GetWindowTextLengthW(hMainEdit) + 1;
	LPWSTR wsBuff = new WCHAR[lng];
	GetWindowTextW(hMainEdit, wsBuff, lng);

	switch (encoding)
	{
	case ENCODING_UTF8:
		{
			//Save as "UTF-8" encoded text file
			int count = WideCharToMultiByte(CP_UTF8, 0, wsBuff, -1, nullptr, 0, nullptr, nullptr);
			char* buff = new char[count];
			WideCharToMultiByte(CP_UTF8, 0, wsBuff, -1, buff, count, nullptr, nullptr);
			stream << buff;
			delete[] buff;
			SendMessageW(hStatus, SB_SETTEXTW, MAKEWORD(4, 0), reinterpret_cast<LPARAM>(L"UTF-8"));
			break;
		}
	case ENCODING_UTF16_LE:
		{
			//Save as "UTF-16 Litle Endian" encoded text file

			uint16_t BOM = 0xFEFF;
			stream.write(reinterpret_cast<char*>(&BOM), sizeof(uint16_t));
			stream.write(reinterpret_cast<char*>(wsBuff), (lng - 1) * sizeof(WCHAR));
			SendMessageW(hStatus, SB_SETTEXTW, MAKEWORD(4, 0), reinterpret_cast<LPARAM>(L"UTF-16 LE"));
			break;
		}
	case ENCODING_UTF16_BE:
		{
			//Save as "UTF-16 Big Endian" encoded text file

			uint16_t BOM = 0xFFFE;
			stream.write(reinterpret_cast<char*>(&BOM), sizeof(uint16_t));
			for (int i = 0; i < lng; i++)
			{
				wsBuff[i] = _byteswap_ushort(wsBuff[i]);
			}
			stream.write(reinterpret_cast<char*>(wsBuff), (lng - 1) * sizeof(WCHAR));
			SendMessageW(hStatus, SB_SETTEXTW, MAKEWORD(4, 0), reinterpret_cast<LPARAM>(L"UTF-16 BE"));
			break;
		}
	case ENCODING_UTF8_BOM:
		{
			//Save as "UTF-8 BOM" encoded text file
			BYTE BOM[] = { 0xEF, 0xBB, 0xBF };
			stream.write(reinterpret_cast<char*>(BOM), sizeof(BOM));
			int count = WideCharToMultiByte(CP_UTF8, 0, wsBuff, -1, nullptr, 0, nullptr, nullptr);
			char* buff = new char[count];
			WideCharToMultiByte(CP_UTF8, 0, wsBuff, -1, buff, count, nullptr, nullptr);
			stream << buff;
			delete[] buff;
			SendMessageW(hStatus, SB_SETTEXTW, MAKEWORD(4, 0), reinterpret_cast<LPARAM>(L"UTF-8 BOM"));
			break;
		}
	}
	stream.close();
	delete[] wsBuff;
}
