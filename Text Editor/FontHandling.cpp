#include  "pch.h"
#include "ProgramHeader.h"
void SetDefaultFont(LPLOGFONTW lpLf)
{
	lpLf->lfHeight = 30;
	lpLf->lfWidth = 0;
	lpLf->lfEscapement = 0;
	lpLf->lfOrientation = 0;
	lpLf->lfWeight = FW_DONTCARE;
	lpLf->lfItalic = FALSE;
	lpLf->lfUnderline = FALSE;
	lpLf->lfStrikeOut = FALSE;
	lpLf->lfCharSet = ANSI_CHARSET;
	lpLf->lfOutPrecision = OUT_TT_PRECIS;
	lpLf->lfClipPrecision = CLIP_DEFAULT_PRECIS;
	lpLf->lfQuality = DEFAULT_QUALITY;
	lpLf->lfPitchAndFamily = DEFAULT_PITCH | FF_DONTCARE;
	lstrcpyW(lpLf->lfFaceName, L"Consolas");
}

bool WriteFontToRegistry(HKEY hReg, LPLOGFONTW lpLf)
{
	DWORD data;
	data = lpLf->lfHeight;
	if (RegSetKeyValueW(hReg, nullptr, (L"lfHeight"), 
		REG_DWORD, &data,
		sizeof(data)) != ERROR_SUCCESS
		) return false;

	data = lpLf->lfWidth;
	if (RegSetKeyValueW(hReg, nullptr, (L"lfWidth"), 
		REG_DWORD, &data,
		sizeof(data)) != ERROR_SUCCESS
		) return false;

	data = lpLf->lfEscapement;
	if (RegSetKeyValueW(hReg, nullptr, (L"lfEscapement"), 
		REG_DWORD, &data,
		sizeof(data)) != ERROR_SUCCESS
		) return false;

	data = lpLf->lfOrientation;
	if (RegSetKeyValueW(hReg, nullptr, (L"lfOrientation"), 
		REG_DWORD, &data,
		sizeof(data)) != ERROR_SUCCESS
		) return false;

	data = lpLf->lfWeight;
	if (RegSetKeyValueW(hReg, nullptr, (L"lfWeight"), 
		REG_DWORD, &data,
		sizeof(data)) != ERROR_SUCCESS
		) return false;

	data = lpLf->lfItalic;
	if (RegSetKeyValueW(hReg, nullptr, (L"lfItalic"), 
		REG_DWORD, &data,
		sizeof(data)) != ERROR_SUCCESS
		) return false;

	data = lpLf->lfUnderline;
	if (RegSetKeyValueW(hReg, nullptr, (L"lfUnderline"),  
		REG_DWORD, &data, 
		sizeof(data)) != ERROR_SUCCESS
		) return false;

	data = lpLf->lfStrikeOut;
	if (RegSetKeyValueW(hReg, nullptr, (L"lfStrikeOut"),
		REG_DWORD, &data ,
		sizeof(data)) != ERROR_SUCCESS
		) return false;

	data = lpLf->lfCharSet;
	if (RegSetKeyValueW(hReg, nullptr, (L"lfCharSet"), 
		REG_DWORD, &data, 
		sizeof(data)) != ERROR_SUCCESS
		) return false;

	data = lpLf->lfOutPrecision;
	if (RegSetKeyValueW(hReg, nullptr, (L"lfOutPrecision"), 
		REG_DWORD, &data, 
		sizeof(data)) != ERROR_SUCCESS
		) return false;

	data = lpLf->lfQuality;
	if (RegSetKeyValueW(hReg, nullptr, (L"lfQuality"),  
		REG_DWORD, &data, 
		sizeof(data)) != ERROR_SUCCESS
		) return false;

	data = lpLf->lfPitchAndFamily;
	if (RegSetKeyValueW(hReg, nullptr, (L"lfPitchAndFamily"),  
		REG_DWORD, &data, 
		sizeof(data)) != ERROR_SUCCESS
		) return false;

	if (RegSetKeyValueW(hReg, nullptr, (L"lfFaceName"), 
		REG_SZ, (&(lpLf->lfFaceName)), 
		sizeof(lpLf->lfFaceName)) != ERROR_SUCCESS
		) return false;

	return true;
}

bool ReadFontFromRegistry(HKEY hReg, LPLOGFONTW lpLf)
{
	DWORD dataSize = sizeof(DWORD);
	DWORD data = 0;

	if (RegGetValueW(hReg, nullptr, (L"lfHeight"), RRF_RT_ANY,
		nullptr, &(lpLf->lfHeight),
		&dataSize) != ERROR_SUCCESS
		) return false;

	if (RegGetValueW(hReg, nullptr, (L"lfWidth"), RRF_RT_ANY,
		nullptr, &(lpLf->lfWidth),
		&dataSize) != ERROR_SUCCESS
		) return false;

	if (RegGetValueW(hReg, nullptr, (L"lfEscapement"), RRF_RT_ANY,
		nullptr, &(lpLf->lfEscapement),
		&dataSize) != ERROR_SUCCESS
		) return false;

	if (RegGetValueW(hReg, nullptr, (L"lfOrientation"), RRF_RT_ANY,
		nullptr, &(lpLf->lfOrientation),
		&dataSize) != ERROR_SUCCESS
		) return false;

	if (RegGetValueW(hReg, nullptr, (L"lfWeight"), RRF_RT_ANY,
		nullptr, &(lpLf->lfWeight),
		&dataSize) != ERROR_SUCCESS
		) return false;

	//BYTE values
	if (RegGetValueW(hReg, nullptr, (L"lfItalic"), RRF_RT_ANY,
		nullptr, &data,
		&dataSize) != ERROR_SUCCESS
		) return false;
	lpLf->lfItalic = static_cast<BYTE>(data);

	if (RegGetValueW(hReg, nullptr, (L"lfUnderline"), RRF_RT_ANY,
		nullptr, &data,
		&dataSize) != ERROR_SUCCESS
		) return false;
	lpLf->lfUnderline = static_cast<BYTE>(data);

	if (RegGetValueW(hReg, nullptr, (L"lfStrikeOut"), RRF_RT_ANY,
		nullptr, &data,
		&dataSize) != ERROR_SUCCESS
		) return false;
	lpLf->lfStrikeOut = static_cast<BYTE>(data);

	if (RegGetValueW(hReg, nullptr, (L"lfCharSet"), RRF_RT_ANY,
		nullptr, (&lpLf->lfCharSet),
		&dataSize) != ERROR_SUCCESS
		) return false;
	lpLf->lfCharSet = static_cast<BYTE>(data);

	if (RegGetValueW(hReg, nullptr, (L"lfOutPrecision"), RRF_RT_ANY,
		nullptr, (&lpLf->lfOutPrecision),
		&dataSize) != ERROR_SUCCESS
		) return false;
	lpLf->lfOutPrecision = static_cast<BYTE>(data);

	if (RegGetValueW(hReg, nullptr, (L"lfQuality"), RRF_RT_ANY,
		nullptr, (&lpLf->lfQuality),
		&dataSize) != ERROR_SUCCESS
		) return false;
	lpLf->lfQuality = static_cast<BYTE>(data);

	if (RegGetValueW(hReg, nullptr, (L"lfPitchAndFamily"), RRF_RT_ANY,
		nullptr, (&lpLf->lfPitchAndFamily),
		&dataSize) != ERROR_SUCCESS
		) return false;
	lpLf->lfPitchAndFamily = static_cast<BYTE>(data);;

	dataSize = sizeof(lpLf->lfFaceName);
	if (RegGetValueW(hReg, nullptr, (L"lfFaceName"), RRF_RT_ANY,
		nullptr, (&lpLf->lfFaceName),
		&dataSize) != ERROR_SUCCESS
		) return false;

	return true;
}

HFONT FontFromLogFont(LPLOGFONTW lpLf)
{
	return CreateFontW(lpLf->lfHeight, lpLf->lfWidth, lpLf->lfEscapement,
		lpLf->lfOrientation, lpLf->lfWeight, lpLf->lfItalic, lpLf->lfUnderline, lpLf->lfStrikeOut,
		lpLf->lfCharSet, lpLf->lfOutPrecision, lpLf->lfClipPrecision, lpLf->lfQuality,
		lpLf->lfPitchAndFamily, lpLf->lfFaceName);
}
