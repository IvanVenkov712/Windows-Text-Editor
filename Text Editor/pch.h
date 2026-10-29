/** @file pch.h
 * @brief Shared Windows/C++ includes and the common-controls v6 manifest.
 * @note Despite its name, this is not configured as a precompiled header.
 */
#pragma once

//Enable Visual styles
#pragma comment(linker,"\"/manifestdependency:type='win32' \
name='Microsoft.Windows.Common-Controls' version='6.0.0.0' \
processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

#define STRICT_TYPED_ITEMIDS
#include <shlobj.h>
#include <objbase.h>      // For COM headers
#include <shobjidl.h>     // for IFileDialogEvents and IFileDialogControlEvents
#include <shlwapi.h>
#include <knownfolders.h> // for KnownFolder APIs/datatypes/function headers
#include <propvarutil.h>  // for PROPVAR-related functions
#include <propkey.h>      // for the Property key APIs/datatypes
#include <propidl.h>      // for the Property System APIs
#include <strsafe.h>      // for StringCchPrintfW
#include <shtypes.h>      // for COMDLG_FILTERSPEC
#include <windows.h>
#include <Richedit.h>
#include <TextServ.h>
#include <shellapi.h>

#include <tchar.h>
#include <string>
#include <sstream>
#include <fstream>
#include <regex>
#include <exception>
#include <filesystem>
#include <new>
