// Header containing macros and declarations of functions and classes used in the program
#pragma once
#include "resource.h"
#define ID_MAIN_EDIT 103
#define ID_STATUS 110
#define CONTROL_GROUP 104
#define CONTROL_LIST 105
#define ENCODING_UTF8 106
#define ENCODING_UTF16_LE 107
#define ENCODING_UTF16_BE 108
#define ENCODING_UTF8_BOM 109
#define ENCODING_AUTO_FIND 110
#define CLASS_NAME TEXT("MainWindowClass")
#define WindowWidth (800)
#define WindowHeight (600)
#define TextLimit MAXINT
#define _WPARAM(value) (reinterpret_cast<WPARAM>(value))
#define _LPARAM(value) (reinterpret_cast<LPARAM>(value))

typedef LRESULT (* PHANDLEUNSAVEDFUNC)(LPARAM lParam);

//Global variables
extern bool haveUnsaved; // True if there are unsaved changes in the current working file
extern UINT encoding;    // Indicates which encoding is chosen to display or save the working file
extern LPWSTR fileName;  // The name of the current working file
extern HWND hWndMain;    // Handle to the main application window
extern HWND hMainEdit;       // Handle to the edit control where the file is edited
extern HWND hDirEdit;        // Handle to the edit control that displays the current working file
extern HWND hStatus;
extern HFONT hMainEditFont;

LRESULT  ExecuteHandleUnsaved(PHANDLEUNSAVEDFUNC Func, LPARAM lParam);

LRESULT OpenWorkingFile();
void LoadBufferUTF_8(char* buff);
void LoadBufferUTF_16_LE(char* buff);
void LoadBufferUTF_16_BE(char* buff, UINT64 fileSize);

void SaveWorkingFile();

LRESULT ShowOpenFileDialog(LPARAM lParam);
HRESULT ShowSaveFileDialog();

void SetDefaultFont(LPLOGFONTW lpLf);
bool WriteFontToRegistry(HKEY hReg, LPLOGFONTW lpLf);
bool ReadFontFromRegistry(HKEY hReg, LPLOGFONTW lpLf);
HFONT FontFromLogFont(LPLOGFONTW lpLf);
LSTATUS OpenDefRegKey(PHKEY lpRegOut, LPDWORD lpDisposition);

// Procedure of the main window
// Definition in "Source.cpp"
LRESULT CALLBACK WindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK EditControlProc(HWND hWnd, UINT uMsg, WPARAM wParam,
	LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData);
INT_PTR CALLBACK GotoProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);