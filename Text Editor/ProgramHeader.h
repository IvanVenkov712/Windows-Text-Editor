/**
 * @file ProgramHeader.h
 * @brief Shared Win32 identifiers, document state, and cross-module contracts.
 *
 * Include pch.h first for Windows types. Functions operate on the globals below
 * and are called synchronously on the UI thread. See docs/ARCHITECTURE.md.
 */
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

/// Action invoked by ExecuteHandleUnsaved after the user permits continuation.
typedef LRESULT (* PHANDLEUNSAVEDFUNC)(LPARAM lParam);

//Global variables
extern bool haveUnsaved; // True if there are unsaved changes in the current working file
extern UINT encoding;    // Indicates which encoding is chosen to display or save the working file
extern LPWSTR fileName;  // Active path; allocation/ownership varies by entry path (see known issues).
extern HWND hWndMain;    // Handle to the main application window
extern HWND hMainEdit;       // Handle to the edit control where the file is edited
extern HWND hDirEdit;        // Handle to the edit control that displays the current working file
extern HWND hStatus;
extern HFONT hMainEditFont;

/**
 * @brief Prompt to save a dirty document before invoking an action.
 * @param Func Non-null callback for New, Open, drop, or Close.
 * @param lParam Context forwarded unchanged to Func.
 * @return Callback result, or zero when canceled / Save As fails.
 * @warning SaveWorkingFile cannot report write failure, so continuation is not
 * proof of a successful save. In the dirty branch the flag is cleared after Func.
 */
LRESULT  ExecuteHandleUnsaved(PHANDLEUNSAVEDFUNC Func, LPARAM lParam);

/**
 * @brief Read fileName into hMainEdit and update path, Save, and status UI.
 * @return Encoding ID used, after BOM/heuristic detection when requested.
 * @warning Requires a valid path and live controls. File/allocation errors can
 * throw; stream failures are unchecked. Does not explicitly clear haveUnsaved.
 */
LRESULT OpenWorkingFile();

/**
 * @brief Decode NUL-terminated UTF-8, normalize to NFC, and update the editor.
 * @param buff Borrowed UTF-8 bytes, without a leading BOM; not modified.
 * @warning Conversion and normalization errors are not checked.
 */
void LoadBufferUTF_8(char* buff);

/**
 * @brief Load NUL-terminated UTF-16 LE bytes, skipping a leading BOM.
 * @param buff Borrowed buffer suitable for WCHAR access; not modified.
 */
void LoadBufferUTF_16_LE(char* buff);

/**
 * @brief Swap UTF-16 BE code units in place, skip the BOM, and load the editor.
 * @param buff Writable buffer suitable for WCHAR access, with trailing zeros.
 * @param fileSize Byte count excluding the trailing zero padding.
 * @warning The 16-bit loop index wraps at 65,536 code units; large inputs hang.
 */
void LoadBufferUTF_16_BE(char* buff, UINT64 fileSize);

/**
 * @brief Truncate fileName and serialize hMainEdit using the global encoding.
 * @warning Requires a valid path and an explicit encoding. Write failures are
 * not reported; this routine does not reset haveUnsaved or perform atomic saves.
 */
void SaveWorkingFile();

/**
 * @brief Show a COM Open dialog with encoding selection, then load its result.
 * @param lParam Unused; retained for PHANDLEUNSAVEDFUNC compatibility.
 * @return Dialog/setup HRESULT cast to LRESULT, including cancellation failures.
 * @warning Does not prompt for unsaved edits itself; loading may throw.
 */
LRESULT ShowOpenFileDialog(LPARAM lParam);

/**
 * @brief Show Save As (default UTF-8), install the path, and write the document.
 * @return Dialog/setup HRESULT; success does not validate the actual file write.
 * @warning Clears haveUnsaved after calling the unchecked writer.
 */
HRESULT ShowSaveFileDialog();

/// Populate a writable LOGFONTW with the default 30-unit Consolas font.
void SetDefaultFont(LPLOGFONTW lpLf);

/**
 * @brief Write individual font fields to a caller-owned open registry key.
 * @return True if every attempted value write succeeds; partial writes remain.
 * @note lfClipPrecision is not persisted by the current implementation.
 */
bool WriteFontToRegistry(HKEY hReg, LPLOGFONTW lpLf);

/**
 * @brief Read persisted font fields into a caller-owned LOGFONTW.
 * @return False on the first failed read; the output may be partly populated.
 * @warning Some BYTE fields use incorrect read destinations/stale data, and
 * lfClipPrecision is omitted. See docs/KNOWN_ISSUES.md before reusing this format.
 */
bool ReadFontFromRegistry(HKEY hReg, LPLOGFONTW lpLf);

/**
 * @brief Create a GDI font from a borrowed LOGFONTW.
 * @return Font handle, or null on failure; caller must eventually DeleteObject.
 */
HFONT FontFromLogFont(LPLOGFONTW lpLf);

/**
 * @brief Create or open HKEY_CURRENT_USER\\TextEditor with KEY_ALL_ACCESS.
 * @param lpRegOut Receives the key; caller must RegCloseKey on success.
 * @param lpDisposition Optional creation/open disposition output.
 * @return Registry API status code.
 */
LSTATUS OpenDefRegKey(PHKEY lpRegOut, LPDWORD lpDisposition);

/// Dispatch main-window messages, menu commands, and find/replace notifications.
LRESULT CALLBACK WindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

/// Delegate edit messages first, then refresh caret, selection, and zoom UI.
LRESULT CALLBACK EditControlProc(HWND hWnd, UINT uMsg, WPARAM wParam,
	LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData);

/// Resolve a one-based line input to nGotoChInd and end the modal dialog on OK.
INT_PTR CALLBACK GotoProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
