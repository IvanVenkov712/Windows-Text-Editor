#include "pch.h"
#include "ProgramHeader.h"

LRESULT InitProgram(HWND hWnd);
void OpenDropFile(HDROP hDFiles);
void FindReplaceProc(LPFINDREPLACE lpfr);
void ChangeStatusBarState(HWND hWnd);
void SwapMainEditControls(HWND hWnd);
void WriteDateHour();
void SetEditProgramDefaults(HWND hEditControl, HFONT hDefFont);
//Global variables
bool haveUnsaved;
bool fontChanged;
bool isVisibleStatBar;
UINT encoding;    
LPWSTR fileName;
HWND hWndMain;   
HWND hMainEdit;      
HWND hDirEdit;       
HWND hStatus;
HWND hFindText;

HFONT hMainEditFont, hDirEditFont;
HINSTANCE hInst;
LOGFONTW lf;
int nGotoChInd;
FINDREPLACEW fr;
WCHAR findWhat[128];
WCHAR replaceWith[128];
UINT idFindMsg = 0;

// Entry point
int APIENTRY wWinMain(
	_In_ HINSTANCE hInstance,
	_In_opt_ HINSTANCE hPrevInstance,
	_In_ LPWSTR lpCmdLine,
	_In_ int nShowCmd
)
{
	UNREFERENCED_PARAMETER(hPrevInstance);

	hInst = hInstance;
	haveUnsaved = false;
	fontChanged = false;
	isVisibleStatBar = true;
	encoding = ENCODING_AUTO_FIND;
	fileName = nullptr;
	lf = {};
	DWORD dwDisposition = 0;
	HKEY hRegKey;
	LSTATUS kStat = OpenDefRegKey(&hRegKey, &dwDisposition);

	if (kStat != ERROR_SUCCESS)	SetDefaultFont(&lf);
	else if (dwDisposition == REG_CREATED_NEW_KEY)
	{
		SetDefaultFont(&lf);
		WriteFontToRegistry(hRegKey, &lf);
		RegCloseKey(hRegKey);
	}
	else if (dwDisposition== REG_OPENED_EXISTING_KEY)
	{
		if (!ReadFontFromRegistry(hRegKey, &lf))SetDefaultFont(&lf);
		RegCloseKey(hRegKey);
	}

	INITCOMMONCONTROLSEX iccEx{
		.dwSize =  sizeof(INITCOMMONCONTROLSEX),
		.dwICC = ICC_BAR_CLASSES | ICC_STANDARD_CLASSES
	};
	if (!InitCommonControlsEx(&iccEx)) ExitProcess(0);

	idFindMsg = RegisterWindowMessageW(FINDMSGSTRINGW);
	if (!idFindMsg) return 0;

	// Initialization of the main class
	WNDCLASSEXW clsMain{
		.cbSize = sizeof(WNDCLASSEXW),
		.lpfnWndProc = WindowProc,
		.hInstance = hInstance,
		.hCursor = LoadCursor(nullptr, (IDC_ARROW)),
		.lpszMenuName = MAKEINTRESOURCEW(IDM_MAINMENU),
		.lpszClassName = CLASS_NAME,
	};
	if (!RegisterClassExW(&clsMain)) ExitProcess(0);

	hWndMain = CreateWindowExW(
		WS_EX_ACCEPTFILES, CLASS_NAME, L"Text Editor",
		WS_OVERLAPPEDWINDOW,
		(GetSystemMetrics(SM_CXSCREEN) - WindowWidth) / 2,
		(GetSystemMetrics(SM_CYSCREEN) - WindowHeight) / 2,
		WindowWidth, WindowHeight, nullptr, nullptr,
		hInst, nullptr
	);
	if (hWndMain == nullptr) ExitProcess(0);

	ShowWindow(hWndMain, nShowCmd);
	UpdateWindow(hWndMain);

	SetEditProgramDefaults(hMainEdit, hMainEditFont);

	SendMessageW(hDirEdit, WM_SETFONT, _WPARAM(hDirEditFont), TRUE);
	SendMessageW(hStatus, SB_SETTEXTW, MAKEWORD(1, 0), _LPARAM(L"Ln 1, Ch 1"));
	SendMessageW(hStatus, SB_SETTEXTW, MAKEWORD(2, 0), _LPARAM(L"100%"));
	SendMessageW(hStatus, SB_SETTEXTW, MAKEWORD(3, 0), _LPARAM(L"Windows (CRLF)"));
	SendMessageW(hStatus, SB_SETTEXTW, MAKEWORD(4, 0), _LPARAM(L"UTF-8"));
	SetFocus(hMainEdit);
	
	HACCEL hAccel = LoadAcceleratorsW(hInst, MAKEINTRESOURCEW(IDR_ACCELERATOR1));
	if (!hAccel) ExitProcess(0);

	int argc = 0;
	LPWSTR* argv = CommandLineToArgvW(lpCmdLine, &argc);
	if (argc == 1)
	{
		fileName = argv[0];
		OpenWorkingFile();
	}
	//Message loop of the main window
	MSG msg;
	while (GetMessageW(&msg, nullptr, 0, 0) > 0)
	{
		if (!(TranslateAcceleratorW(hWndMain, hAccel, &msg) || IsDialogMessageW(hFindText, &msg)))
		{
			TranslateMessage(&msg);
			DispatchMessageW(&msg);
		}
	}
	ExitProcess(0);
}
LRESULT InitProgram(HWND hWnd)
{
	hDirEdit = CreateWindowExW(
		0, WC_EDIT, L"new",
		WS_BORDER | WS_CHILD | WS_VISIBLE | ES_LEFT | ES_AUTOHSCROLL | ES_READONLY,
		10, 0, WindowWidth - 20, 35,
		hWnd, nullptr, hInst, nullptr
	);
	if (hDirEdit == nullptr) return -1;

	hMainEdit = CreateWindowExW(
		0,
		WC_EDIT,
		L"",
		WS_CHILD | WS_VISIBLE | WS_VSCROLL | WS_HSCROLL | ES_LEFT | ES_MULTILINE | ES_AUTOVSCROLL | ES_AUTOHSCROLL | ES_NOHIDESEL,
		0, 40, WindowWidth, WindowHeight - 63,
		hWnd, (HMENU)ID_MAIN_EDIT, hInst, nullptr
	);
	if (hMainEdit == nullptr) return -1;
	if (!SetWindowSubclass(hMainEdit, EditControlProc, 0, 0)) return -1;

	hStatus = CreateWindowExW(
		0, STATUSCLASSNAME, nullptr,
		SBARS_SIZEGRIP | WS_CHILD | WS_VISIBLE, 0, 0, 0, 0, hWnd,
		(HMENU)ID_STATUS, hInst, NULL
	);
	if (hStatus == nullptr)return -1;
	fr = {
		.lStructSize = sizeof(FINDREPLACEW),
		.hwndOwner = hWnd,
		.hInstance = hInst,
		.Flags = FR_HIDEWHOLEWORD | FR_HIDEUPDOWN,
		.lpstrFindWhat = findWhat,
		.lpstrReplaceWith = replaceWith,
		.wFindWhatLen = sizeof(findWhat),
		.wReplaceWithLen = sizeof(replaceWith),
	};

	hMainEditFont = FontFromLogFont(&lf);
	if (hMainEditFont == nullptr) return -1;

	hDirEditFont = CreateFontW(30, 0, 0, 0, FW_DONTCARE, FALSE, FALSE, FALSE, ANSI_CHARSET,
		OUT_TT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
		DEFAULT_PITCH | FF_DONTCARE, (L"Consolas"));
	if (hDirEditFont == nullptr) return -1;
	return 0;
}
void SetEditProgramDefaults(HWND hEditControl, HFONT hDefFont)
{
	SendMessageW(hEditControl, WM_SETFONT, _WPARAM(hDefFont), TRUE);
	SendMessageW(hEditControl, EM_SETEXTENDEDSTYLE,
		ES_EX_ZOOMABLE | ES_EX_ALLOWEOL_ALL,
		ES_EX_ZOOMABLE | ES_EX_ALLOWEOL_ALL);
	SendMessageW(hEditControl, EM_SETLIMITTEXT, TextLimit, 0);
	SendMessageW(hEditControl, EM_SETZOOM, 100, 100);
}
LRESULT CALLBACK WindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	if (uMsg == idFindMsg)
	{
		FindReplaceProc(reinterpret_cast<LPFINDREPLACE>(lParam));
		return 0;
	}
	switch (uMsg)
	{
	case WM_CREATE:
		return InitProgram(hWnd);
	case WM_DESTROY:
		PostQuitMessage(0);
		return 0;
	case WM_CLOSE:
		{
			auto func = [](LPARAM lParam)->LRESULT
			{
				if (fontChanged)
				{
					HKEY hRegKey;
					if (OpenDefRegKey(&hRegKey, nullptr) == ERROR_SUCCESS)
					{
						WriteFontToRegistry(hRegKey, &lf);
						RegCloseKey(hRegKey);
					}
				}
				DestroyWindow(reinterpret_cast<HWND>(lParam));
				return 0;
			};
			PHANDLEUNSAVEDFUNC toExec = static_cast<PHANDLEUNSAVEDFUNC>(func);
			ExecuteHandleUnsaved(toExec, _LPARAM(hWnd));
			return 0;
		}
	case WM_PAINT:
		{
			PAINTSTRUCT ps;
			HDC hdc = BeginPaint(hWnd, &ps);
			FillRect(hdc, &ps.rcPaint, (HBRUSH)(COLOR_WINDOW + 1));
			EndPaint(hWnd, &ps);
			return 0;
		}
	case WM_SIZE:
		{
			int newWidth = LOWORD(lParam);
			int rightEdgesStatus[] = { newWidth - 430, newWidth - 290, newWidth - 240, newWidth - 120, newWidth };
			if (isVisibleStatBar)SetWindowPos(hMainEdit, NULL, 0, 40, newWidth, HIWORD(lParam) - 63, SWP_SHOWWINDOW);
			else SetWindowPos(hMainEdit, NULL, 0, 40, newWidth, HIWORD(lParam) - 40, SWP_SHOWWINDOW);
			SetWindowPos(hDirEdit, NULL, 10, 0, newWidth - 20, 35, SWP_SHOWWINDOW);
			SendMessageW(hStatus, WM_SIZE, 0, 0);
			SendMessageW(hStatus, SB_SETPARTS, 5, (LPARAM)rightEdgesStatus);
			return 0;
		}
	case WM_DROPFILES:
		{
			OpenDropFile(reinterpret_cast<HDROP>(wParam));
			return 0;
		}
	case WM_COMMAND:
		{
			switch (LOWORD(wParam))
			{
			case ID_ACCELNEWWINDOW:
				[[fallthrough]];
			case ID_FILE_NEWWINDOW:
				{
					STARTUPINFO si{.cb = sizeof(STARTUPINFO)};
					PROCESS_INFORMATION pi{};
					TCHAR processName[MAX_PATH];
					GetModuleFileNameW(nullptr, processName, MAX_PATH);
					CreateProcessW(
						processName, nullptr, nullptr,
						nullptr, FALSE, 0, nullptr,
						nullptr, &si, &pi
					);
					CloseHandle(pi.hThread);
					CloseHandle(pi.hProcess);
					return 0;
				}
			case ID_ACCELNEW:
				[[fallthrough]];
			case ID_FILE_NEW:
				{
					auto func = [](LPARAM lParam)->LRESULT
					{
						UNREFERENCED_PARAMETER(lParam);
						SetWindowTextW(hMainEdit, L"");
						SetWindowTextW(hDirEdit, L"new");
						SendMessageW(hStatus, SB_SETTEXTW, MAKEWORD(1, 0), _LPARAM(L"Ln 1, Ch 1"));
						SendMessageW(hStatus, SB_SETTEXTW, MAKEWORD(3, 0), _LPARAM(L"Windows (CRLF)"));
						SendMessageW(hStatus, SB_SETTEXTW, MAKEWORD(4, 0), _LPARAM(L"UTF-8"));
						SetFocus(hMainEdit);
						return 0;
					};
					PHANDLEUNSAVEDFUNC toExec = static_cast<PHANDLEUNSAVEDFUNC>(func);
					ExecuteHandleUnsaved(toExec, 0);
					return 0;
				}
			case ID_FILE_EXIT:
				SendMessageW(hWnd, WM_CLOSE, 0, 0);
				return 0;
			case ID_ACCELOPEN:
				[[fallthrough]];
			case ID_FILE_OPEN:
				ExecuteHandleUnsaved(ShowOpenFileDialog, 0);
				return 0;
			case ID_ACCELSAVE:
				[[fallthrough]];
			case ID_FILE_SAVE:
				{	
					MENUITEMINFOW mii{
						.cbSize = sizeof(MENUITEMINFOW),
						.fMask = MIIM_STATE
					};
					GetMenuItemInfoW(GetMenu(hWnd), ID_FILE_SAVE, FALSE, &mii);
					if ((!(mii.fState & (MFS_DISABLED))) && haveUnsaved)
						SaveWorkingFile();
					return 0;
				}
			case ID_ACCELSAVEAS:
				[[fallthrough]];
			case ID_FILE_SAVEAS:
				ShowSaveFileDialog();
				return 0;
			case ID_EDIT_UNDO:
				SendMessageW(hMainEdit, EM_UNDO, 0, 0);
				return 0;
			case ID_EDIT_CUT:
				SendMessageW(hMainEdit, WM_CUT, 0, 0);
				return 0;
			case ID_EDIT_COPY:
				SendMessageW(hMainEdit, WM_COPY, 0, 0);
				return 0;
			case ID_EDIT_PASTE:
				SendMessageW(hMainEdit, WM_PASTE, 0, 0);
				return 0;
			case ID_EDIT_DELETE:
				SendMessageW(hMainEdit, WM_CLEAR, 0, 0);
				return 0;
			case ID_ACCELFIND:
				[[fallthrough]];
			case ID_EDIT_FIND:
				hFindText = FindTextW(&fr);
				return 0;
			case ID_ACCELREPLACE:
				[[fallthrough]];
			case ID_EDIT_REPLACE:
				hFindText = ReplaceTextW(&fr);
				return 0;
			case ID_ACCELGOTO:
				[[fallthrough]];
			case ID_EDIT_GOTO:
				{
					INT_PTR res = DialogBoxW(hInst, MAKEINTRESOURCEW(IDD_GOTO), hWnd, GotoProc);
					if (res == IDOK)
					{
						SetFocus(hMainEdit);
						SendMessageW(hMainEdit, EM_SETSEL, nGotoChInd, nGotoChInd);
					}
					return 0;
				}
			case ID_EDIT_SELECTALL:
				SendMessageW(hMainEdit, EM_SETSEL, 0, -1);
				return 0;
			case ID_ACCELDATEHOUR:
				[[fallthrough]];
			case ID_EDIT_HOUR:
				WriteDateHour();
				return 0;
			case ID_FORMAT_WORDWRAP:
				SwapMainEditControls(hWnd);
				return 0;
			case ID_FORMAT_FONT:
				{
					CHOOSEFONTW cf{
						.lStructSize = sizeof(CHOOSEFONTW),
						.hwndOwner = hWnd,
						.lpLogFont = &lf,
						.iPointSize = 1,
						.Flags = CF_INITTOLOGFONTSTRUCT | CF_FORCEFONTEXIST | CF_SCALABLEONLY | CF_NOVERTFONTS,
						.nFontType = SCREEN_FONTTYPE
					};
					if (ChooseFontW(&cf))
					{
						HFONT hOldFont = hMainEditFont;
						hMainEditFont = FontFromLogFont(&lf);
						if (hMainEditFont)
						{
							DeleteObject(hOldFont);
							fontChanged = true;
						}
						else hMainEditFont = hOldFont;
						SendMessageW(hMainEdit, WM_SETFONT, _WPARAM(hMainEditFont), TRUE);
					}
					return 0;
				}
			case ID_ACCELZOOMINC1:
				[[fallthrough]];
			case ID_ACCELZOOMINC2:
				[[fallthrough]];
			case ID_VIEW_ZOOMIN:
				{
					UINT64 numerator = 0;
					UINT64 denominator = 1;
					SendMessageW(hMainEdit, EM_GETZOOM, _WPARAM(&numerator), _WPARAM(&denominator));
					if (numerator < 500)
					{
						numerator += 10;
						WCHAR statBuff[10];
						SendMessageW(hMainEdit, EM_SETZOOM, numerator, denominator);
						StringCbPrintfW(statBuff, sizeof(statBuff), L"%llu%", numerator);
						SendMessageW(hStatus, SB_SETTEXTW, MAKEWORD(2, 0), _LPARAM(statBuff));
					}
					return 0;
				}
			case ID_ACCELZOOMDEC1:
				[[fallthrough]];
			case ID_ACCELZOOMDEC2:
				[[fallthrough]];
			case ID_VIEW_ZOOMOUT:
				{
					UINT64 numerator = 0;
					UINT64 denominator = 1;
					SendMessageW(hMainEdit, EM_GETZOOM, _WPARAM(&numerator), _WPARAM(&denominator));
					if (numerator > 10)
					{
						numerator -= 10;
						WCHAR statBuff[10];
						SendMessageW(hMainEdit, EM_SETZOOM, numerator, denominator);
						StringCbPrintfW(statBuff, sizeof(statBuff), L"%llu%", numerator);
						SendMessageW(hStatus, SB_SETTEXTW, MAKEWORD(2, 0), _LPARAM(statBuff));
					}
					return 0;
				}
			case ID_ACCELRESTOREZOOM1:
				[[fallthrough]];
			case ID_ACCELRESTOREZOOM2:
				[[fallthrough]];
			case ID_VIEW_RESTOREZOOM:
				SendMessageW(hMainEdit, EM_SETZOOM, 100, 100);
				SendMessageW(hStatus, SB_SETTEXTW, MAKEWORD(2, 0), _LPARAM(L"100%"));
				return 0;
			case ID_VIEW_STATUSBAR:
				ChangeStatusBarState(hWnd);
				return 0;
			case ID_MAIN_EDIT:
				switch (HIWORD(wParam))
				{
				case EN_CHANGE:
					haveUnsaved = true;
					return 0;
				default:
					return DefWindowProcW(hWnd, uMsg, wParam, lParam);
				}
			default:
				return DefWindowProcW(hWnd, uMsg, wParam, lParam);
			}
		}
	default:
		return DefWindowProcW(hWnd, uMsg, wParam, lParam);
	}
}
LRESULT CALLBACK EditControlProc(HWND hWnd, UINT uMsg, WPARAM wParam,
	LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData)
{
	LRESULT res = DefSubclassProc(hWnd, uMsg, wParam, lParam);
	switch (uMsg)
	{
	case WM_MOUSEWHEEL:
		{
			if (GET_KEYSTATE_WPARAM(wParam) == MK_CONTROL)
			{
				UINT64 numerator = 0;
				UINT64 denominator = 1;
				SendMessageW(hWnd, EM_GETZOOM, _WPARAM(&numerator), _WPARAM(&denominator));
				std::wstring ws = std::to_wstring(numerator) + L"%";
				SendMessageW(hStatus, SB_SETTEXTW, MAKEWORD(2, 0), _LPARAM(ws.c_str()));
				break;
			}
		}
	case WM_CHAR:
		[[fallthrough]];
	case WM_KEYDOWN:
		[[fallthrough]];
	case WM_KEYUP:
		[[fallthrough]];
	case WM_LBUTTONDOWN:
		[[fallthrough]];
	case WM_LBUTTONUP:
		[[fallthrough]];
	case WM_LBUTTONDBLCLK:
		[[fallthrough]];
	case WM_RBUTTONDOWN:
		[[fallthrough]];
	case WM_RBUTTONUP:
		[[fallthrough]];
	case WM_RBUTTONDBLCLK:
		[[fallthrough]];
	case WM_MBUTTONDOWN:
		[[fallthrough]];
	case WM_MBUTTONUP:
		[[fallthrough]];
	case WM_MBUTTONDBLCLK:
		[[fallthrough]];
	case EM_SETSEL:
		{
			LRESULT lnNumber = SendMessageW(hWnd, EM_FILELINEFROMCHAR, -1, 0) + 1;
			LRESULT chIndex = SendMessageW(hWnd, EM_GETCARETINDEX, -1, 0) + 1;
			chIndex -= SendMessageW(hWnd, EM_FILELINEINDEX, -1, 0);
			std::wstring ws = L"Ln " + std::to_wstring(lnNumber) + L", Ch " + std::to_wstring(chIndex);
			SendMessageW(hStatus, SB_SETTEXTW, MAKEWORD(1, 0), _LPARAM(ws.c_str()));
			DWORD nBegin = 0, nEnd = 0;
			SendMessageW(hWnd, EM_GETSEL, _WPARAM(&nBegin), _LPARAM(&nEnd));
			if (nBegin != nEnd)
			{
				EnableMenuItem(GetMenu(hWndMain), ID_EDIT_CUT, MF_BYCOMMAND | MF_ENABLED);
				EnableMenuItem(GetMenu(hWndMain), ID_EDIT_COPY, MF_BYCOMMAND | MF_ENABLED);
				EnableMenuItem(GetMenu(hWndMain), ID_EDIT_DELETE, MF_BYCOMMAND | MF_ENABLED);
			}
			else
			{
				EnableMenuItem(GetMenu(hWndMain), ID_EDIT_CUT, MF_BYCOMMAND | MF_DISABLED);
				EnableMenuItem(GetMenu(hWndMain), ID_EDIT_COPY, MF_BYCOMMAND | MF_DISABLED);
				EnableMenuItem(GetMenu(hWndMain), ID_EDIT_DELETE, MF_BYCOMMAND | MF_DISABLED);
			}
			break;
		}
	}
	return res;
}
INT_PTR CALLBACK GotoProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_INITDIALOG:
		SetWindowTextW(GetDlgItem(hWnd, IDGOTO_EDIT), L"1");
		PostMessageW(hWnd, WM_NEXTDLGCTL, _WPARAM(GetDlgItem(hWnd, IDGOTO_EDIT)), TRUE);
		return TRUE;
	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case IDOK:
			{
				int nSize = GetWindowTextLengthW(GetDlgItem(hWnd, IDGOTO_EDIT)) + 1;
				LPWSTR wsBuff = new WCHAR[nSize];
				GetWindowTextW(GetDlgItem(hWnd, IDGOTO_EDIT), wsBuff, nSize);
				int nLnInd = StrToIntW(wsBuff) - 1;
				delete[] wsBuff;
				nGotoChInd = SendMessageW(hMainEdit, EM_FILELINEINDEX, nLnInd, 0);
				if (nGotoChInd == -1)
				{
					MessageBoxW(hWnd, L"The line index is outside the range of lines",
						L"Out of Range", MB_OK);
				}
				else EndDialog(hWnd, LOWORD(wParam));
				return TRUE;
			}
		case IDCANCEL:
			EndDialog(hWnd, LOWORD(wParam));
			return TRUE;
		}
		
	}
	return FALSE;
}

LSTATUS WINAPI OpenDefRegKey(PHKEY lpRegOut, LPDWORD lpDisposition)
{
	return RegCreateKeyExW(HKEY_CURRENT_USER,
		L"TextEditor", 0, nullptr, 0, KEY_ALL_ACCESS,
		nullptr, lpRegOut, lpDisposition
	);
}
void OpenDropFile(HDROP hDFiles)
{
	auto func = [](LPARAM lParam)->LRESULT
	{
		HDROP hDFilesInternal = reinterpret_cast<HDROP>(lParam);
		UINT buffSize = DragQueryFileW(hDFilesInternal, 0, nullptr, 0) + 1;
		LPWSTR oldFileName = fileName;
		fileName = reinterpret_cast<LPWSTR>(CoTaskMemAlloc(sizeof(WCHAR) * buffSize));
		if (fileName)
		{
			if (DragQueryFileW(hDFilesInternal, 0, fileName, buffSize))
			{
				encoding = ENCODING_AUTO_FIND;
				OpenWorkingFile();
				CoTaskMemFree(oldFileName);
			}
			else
			{
				CoTaskMemFree(fileName);
				fileName = oldFileName;
			}
		}
		else fileName = oldFileName;
		return 0;
	};
	PHANDLEUNSAVEDFUNC toExec = static_cast<PHANDLEUNSAVEDFUNC>(func);
	ExecuteHandleUnsaved(toExec, _LPARAM(hDFiles));
	DragFinish(hDFiles);
}
void SwapMainEditControls(HWND hWnd)
{
	MENUITEMINFOW mii{
		.cbSize = sizeof(MENUITEMINFOW),
		.fMask = MIIM_STATE
	};
	GetMenuItemInfoW(GetMenu(hWnd), ID_FORMAT_WORDWRAP, FALSE, &mii);
	DWORD dwNewStyle = 0;
	if (mii.fState == MFS_UNCHECKED)
	{
		dwNewStyle = WS_CHILD | WS_VISIBLE | WS_VSCROLL | ES_LEFT | ES_MULTILINE | ES_AUTOVSCROLL;
		mii.fState = MFS_CHECKED;
	}
	else
	{
		dwNewStyle = WS_CHILD | WS_VISIBLE | WS_VSCROLL | WS_HSCROLL | ES_LEFT | ES_MULTILINE | ES_AUTOVSCROLL | ES_AUTOHSCROLL;
		mii.fState = MFS_UNCHECKED;
	}
	RECT rEdit;
	if (GetWindowRect(hMainEdit, &rEdit))
	{
		HWND hNewEdit = CreateWindowExW(
			0, WC_EDIT, (L""),
			dwNewStyle,
			0, 40,
			rEdit.right - rEdit.left, rEdit.bottom - rEdit.top,
			hWnd, (HMENU)ID_MAIN_EDIT, hInst, nullptr
		);
		if (hNewEdit)
		{
			if (SetWindowSubclass(hNewEdit, EditControlProc, 0, 0))
			{
				int lng = GetWindowTextLengthW(hMainEdit) + 1;
				LPWSTR buff = new WCHAR[lng];
				GetWindowTextW(hMainEdit, buff, lng);
				DWORD nBegin = 0, nEnd = 0;
				UINT64 numerator = 0, denominator = 1;
				SendMessageW(hMainEdit, EM_GETSEL, _WPARAM(&nBegin), _LPARAM(&nEnd));
				SendMessageW(hMainEdit, EM_GETZOOM, _WPARAM(&numerator), _LPARAM(&denominator));
				DestroyWindow(hMainEdit);

				SendMessageW(hNewEdit, WM_SETFONT, _WPARAM(hMainEditFont), TRUE);
				SendMessageW(hNewEdit, EM_SETEXTENDEDSTYLE,
					ES_EX_ZOOMABLE | ES_EX_ALLOWEOL_ALL,
					ES_EX_ZOOMABLE | ES_EX_ALLOWEOL_ALL);
				SendMessageW(hNewEdit, EM_SETLIMITTEXT, TextLimit, 0);
				SendMessageW(hNewEdit, EM_SETZOOM, numerator, denominator);
				SetWindowTextW(hNewEdit, buff);
				SendMessageW(hNewEdit, EM_SETSEL, nBegin, nEnd);
				SetFocus(hNewEdit);
				delete[] buff;
				SetMenuItemInfoW(GetMenu(hWnd), ID_FORMAT_WORDWRAP, FALSE, &mii);
				hMainEdit = hNewEdit;
			}
			else DestroyWindow(hNewEdit);
		}
	}
}
void WriteDateHour()
{
	int ccTime = GetTimeFormatEx(
		LOCALE_NAME_USER_DEFAULT,
		TIME_NOSECONDS, nullptr, nullptr, nullptr, 0
	);
	int ccDate = GetDateFormatEx(
		LOCALE_NAME_USER_DEFAULT,
		DATE_SHORTDATE, nullptr, nullptr, nullptr,
		0, nullptr
	);

	LPWSTR dateTimeString = new WCHAR[ccTime + ccDate];
	GetTimeFormatEx(
		LOCALE_NAME_USER_DEFAULT,
		TIME_NOSECONDS, nullptr, nullptr,
		dateTimeString, ccTime
	);
	dateTimeString[ccTime - 1] = L' ';
	GetDateFormatEx(
		LOCALE_NAME_USER_DEFAULT,
		DATE_SHORTDATE, nullptr, nullptr,
		dateTimeString + ccTime, ccDate, nullptr
	);
	SendMessageW(hMainEdit, EM_REPLACESEL, TRUE, _LPARAM(dateTimeString));
	delete[] dateTimeString;
}
void ChangeStatusBarState(HWND hWnd)
{
	RECT rEdit;
	GetWindowRect(hMainEdit, &rEdit);
	MENUITEMINFOW mii = {};
	mii.cbSize = sizeof(MENUITEMINFOW);
	mii.fMask = MIIM_STATE;
	GetMenuItemInfoW(GetMenu(hWnd), ID_VIEW_STATUSBAR, FALSE, &mii);
	if (mii.fState == MFS_UNCHECKED)
	{
		ShowWindow(hStatus, SW_SHOW);
		rEdit.bottom -= 23;
		mii.fState = MFS_CHECKED;
		isVisibleStatBar = true;
	}
	else
	{
		ShowWindow(hStatus, SW_HIDE);
		rEdit.bottom += 23;
		mii.fState = MFS_UNCHECKED;
		isVisibleStatBar = false;
	}
	SetMenuItemInfoW(GetMenu(hWnd), ID_VIEW_STATUSBAR, FALSE, &mii);
	SetWindowPos(hMainEdit, nullptr, 0, 40,
		rEdit.right - rEdit.left,
		rEdit.bottom - rEdit.top,
		SWP_SHOWWINDOW
	);
}
void FindReplaceProc(LPFINDREPLACE lpfr)
{
	SetCursor(LoadCursorW(nullptr, (IDC_WAIT)));
	if (lpfr->Flags & FR_DIALOGTERM) hFindText = nullptr;
	else
	{
		static int nPos = 0;
		int toFindLen = lstrlenW(lpfr->lpstrFindWhat);
		int editTextLen = GetWindowTextLengthW(hMainEdit) + 1;
		PWSTR(__stdcall * searchFunc)(PCWSTR pszFirst, PCWSTR pszSrch);
		int(__stdcall * cmpFunc)(PCWSTR psz1, PCWSTR psz2);
		int(__stdcall * cmpFuncN)(PCWSTR psz1, PCWSTR psz2, int nChar);
		if (lpfr->Flags & FR_MATCHCASE)
		{
			searchFunc = StrStrW;
			cmpFunc = StrCmpW;
			cmpFuncN = StrCmpNW;
		}
		else
		{
			searchFunc = StrStrIW;
			cmpFunc = StrCmpIW;
			cmpFuncN = StrCmpNIW;
		}
		DWORD nBeginSel = 0, nEndSel = 0;
		bool selIsSearched = false;
		LPCWSTR found = nullptr;
		HLOCAL hl = reinterpret_cast<HLOCAL>(SendMessageW(hMainEdit, EM_GETHANDLE, 0, 0));
		LPCWSTR szBuff = reinterpret_cast<LPCWSTR>(LocalLock(hl));
		if (nPos >= editTextLen) nPos = 0;
		if (szBuff)
		{
			if (lpfr->Flags & (FR_FINDNEXT | FR_REPLACE))
			{
				if (lpfr->Flags & FR_REPLACE)
				{
					SendMessageW(hMainEdit, EM_GETSEL, _WPARAM(&nBeginSel), _LPARAM(&nEndSel));
					if (nBeginSel == nEndSel) selIsSearched = false;
					else
					{
						LPWSTR szSel = new WCHAR[nEndSel - nBeginSel + 1];
						wmemcpy_s(szSel, nEndSel - nBeginSel + 1, szBuff + nBeginSel, nEndSel - nBeginSel);
						szSel[nEndSel - nBeginSel] = L'\0';
						selIsSearched = !(cmpFunc(szSel, lpfr->lpstrFindWhat));
						delete[] szSel;
					}
					if (selIsSearched) SendMessage(hMainEdit, EM_REPLACESEL, TRUE, _LPARAM(lpfr->lpstrReplaceWith));
				}
				found = searchFunc(szBuff + nPos, lpfr->lpstrFindWhat);
				if (!found) found = searchFunc(szBuff, lpfr->lpstrFindWhat);
				if (found)
				{
					nPos = found - szBuff + toFindLen;
					SendMessageW(hMainEdit, EM_SETSEL, found - szBuff, nPos);
					SendMessageW(hMainEdit, EM_SCROLLCARET, 0, 0);
				}
				else MessageBoxW(hFindText, L"String specified could not be found", L"Find", MB_ICONINFORMATION);
				LocalUnlock(hl);
			}
			else if (lpfr->Flags & FR_REPLACEALL)
			{
				int count = 0, nReplacePos = 0;
				int replaceWithLen = lstrlenW(lpfr->lpstrReplaceWith);
				LPCWSTR found, at;
				LPWSTR newText;
				at = szBuff;
				do
				{
					found = searchFunc(at, lpfr->lpstrFindWhat);
					if (found)
					{
						at = found + toFindLen;
						count++;
					}
				} while (found);
				int newTextSize = editTextLen + (replaceWithLen - toFindLen) * count;
				newText = new WCHAR[newTextSize];
				int iNew = 0, iOld = 0;
				while(iNew < newTextSize && iOld < editTextLen)
				{
					if ((editTextLen - iOld < toFindLen) || cmpFuncN(szBuff + iOld, lpfr->lpstrFindWhat, toFindLen))
					{
						newText[iNew] = szBuff[iOld];
						iOld++;
						iNew++;
					}
					else
					{
						wmemcpy_s(newText + iNew, newTextSize - iNew, lpfr->lpstrReplaceWith, replaceWithLen);
						iNew += replaceWithLen;
						iOld += toFindLen;
					}
				}
				newText[newTextSize - 1] = L'\0';
				LocalUnlock(hl);
				SetWindowTextW(hMainEdit, newText);
				delete[] newText;
				haveUnsaved = true;
			}
		}
	}
	SetCursor(LoadCursorW(nullptr, (IDC_ARROW)));
}