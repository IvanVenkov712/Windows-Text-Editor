/** @file FileDialog.cpp
 * @brief Native COM Open and Save As dialogs with custom encoding selection.
 * @details Each call initializes COM, releases interfaces, and uninitializes
 * COM on the UI thread. Public contracts are documented in ProgramHeader.h.
 */
#include "pch.h"
#include "ProgramHeader.h"
LRESULT WINAPI ShowOpenFileDialog(LPARAM lParam)
{
	UNREFERENCED_PARAMETER(lParam);
	HRESULT hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
	if (SUCCEEDED(hr))
	{
		IFileDialog* pfd = NULL;
		hr = CoCreateInstance(CLSID_FileOpenDialog, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&pfd));
		if (SUCCEEDED(hr))
		{
			// Set up a Customization.
			IFileDialogCustomize* pfdc = NULL;
			hr = pfd->QueryInterface(IID_PPV_ARGS(&pfdc));
			if (SUCCEEDED(hr))
			{
				// Create a Visual Group.
				hr = pfdc->StartVisualGroup(CONTROL_GROUP, L"Encoding");
				if (SUCCEEDED(hr))
				{
					// Add a radio-button list.
					hr = pfdc->AddComboBox(CONTROL_LIST);
					if (SUCCEEDED(hr))
					{
						// Set the state of the added radio-button list.
						hr = pfdc->SetControlState(CONTROL_LIST, CDCS_VISIBLE | CDCS_ENABLED);
						if (SUCCEEDED(hr))
						{
							hr = pfdc->AddControlItem(CONTROL_LIST, ENCODING_AUTO_FIND, L"Auto find");
							if (SUCCEEDED(hr))
							{
								// Add individual buttons to the radio-button list.
								hr = pfdc->AddControlItem(CONTROL_LIST, ENCODING_UTF8, L"UTF-8");
								if (SUCCEEDED(hr))
								{
									hr = pfdc->AddControlItem(CONTROL_LIST, ENCODING_UTF16_LE, L"UTF-16 LE");
									if (SUCCEEDED(hr))
									{
										hr = pfdc->AddControlItem(CONTROL_LIST, ENCODING_UTF16_BE, L"UTF-16 BE");
										if (SUCCEEDED(hr))
										{
											hr = pfdc->AddControlItem(CONTROL_LIST, ENCODING_UTF8_BOM, L"UTF-8 BOM");
											if (SUCCEEDED(hr))
											{
												// Set the default selection to option 1.
												hr = pfdc->SetSelectedControlItem(CONTROL_LIST, ENCODING_AUTO_FIND);
											}
										}
									}
								}
							}
						}
					}
					// End the visual group.
					pfdc->EndVisualGroup();
				}
				pfdc->Release();
			}

			if (SUCCEEDED(hr))
			{
				// Now show the dialog.
				hr = pfd->Show(hWndMain);
				if (SUCCEEDED(hr))
				{
					IFileDialogCustomize* pfdc = NULL;
					hr = pfd->QueryInterface(IID_PPV_ARGS(&pfdc));
					if (SUCCEEDED(hr))
					{
						DWORD itemID;
						hr = pfdc->GetSelectedControlItem(CONTROL_LIST, &itemID);
						if (SUCCEEDED(hr))
						{
							encoding = itemID;
							IShellItem* psiResult;
							hr = pfd->GetResult(&psiResult);
							if (SUCCEEDED(hr))
							{
								LPWSTR oldFileName = fileName;
								hr = psiResult->GetDisplayName(SIGDN_FILESYSPATH, &fileName);
								if (SUCCEEDED(hr))
								{
									CoTaskMemFree(oldFileName);
									OpenWorkingFile();
								}
								else fileName = oldFileName;
								psiResult->Release();
							}
						}
						pfdc->Release();
					}
					//
					// You can add your own code here to handle the results.
					//
				}
			}
			pfd->Release();
		}
		CoUninitialize();
	}
	return static_cast<LRESULT>(hr);
}

HRESULT WINAPI ShowSaveFileDialog()
{
	HRESULT hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
	if (SUCCEEDED(hr))
	{
		IFileDialog* pfd = NULL;
		hr = CoCreateInstance(CLSID_FileSaveDialog, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&pfd));
		if (SUCCEEDED(hr))
		{
			// Set up a Customization.
			IFileDialogCustomize* pfdc = NULL;
			hr = pfd->QueryInterface(IID_PPV_ARGS(&pfdc));
			if (SUCCEEDED(hr))
			{
				// Create a Visual Group.
				hr = pfdc->StartVisualGroup(CONTROL_GROUP, L"Encoding");
				if (SUCCEEDED(hr))
				{
					// Add a radio-button list.
					// Add a radio-button list.
					hr = pfdc->AddComboBox(CONTROL_LIST);
					if (SUCCEEDED(hr))
					{
						// Set the state of the added radio-button list.
						hr = pfdc->SetControlState(CONTROL_LIST, CDCS_VISIBLE | CDCS_ENABLED);
						if (SUCCEEDED(hr))
						{
							// Add individual buttons to the radio-button list.
							hr = pfdc->AddControlItem(CONTROL_LIST, ENCODING_UTF8, L"UTF-8");
							if (SUCCEEDED(hr))
							{
								hr = pfdc->AddControlItem(CONTROL_LIST, ENCODING_UTF16_LE, L"UTF-16 LE");
								if (SUCCEEDED(hr))
								{
									hr = pfdc->AddControlItem(CONTROL_LIST, ENCODING_UTF16_BE, L"UTF-16 BE");
									if (SUCCEEDED(hr))
									{
										hr = pfdc->AddControlItem(CONTROL_LIST, ENCODING_UTF8_BOM, L"UTF-8 BOM");
										if (SUCCEEDED(hr))
										{
											// Set the default selection to option 1.
											hr = pfdc->SetSelectedControlItem(CONTROL_LIST, ENCODING_UTF8);
										}
									}
								}
							}
						}
					}
					// End the visual group.
					pfdc->EndVisualGroup();
				}
				pfdc->Release();
			}
			if (SUCCEEDED(hr))
			{
				// Now show the dialog.
				hr = pfd->Show(hWndMain);
				if (SUCCEEDED(hr))
				{
					IFileDialogCustomize* pfdc = NULL;
					hr = pfd->QueryInterface(IID_PPV_ARGS(&pfdc));
					if (SUCCEEDED(hr))
					{
						DWORD itemID;
						hr = pfdc->GetSelectedControlItem(CONTROL_LIST, &itemID);
						if (SUCCEEDED(hr))
						{
							encoding = itemID;
							IShellItem* psiResult;
							hr = pfd->GetResult(&psiResult);
							if (SUCCEEDED(hr))
							{
								LPWSTR oldFileName = fileName;
								hr = psiResult->GetDisplayName(SIGDN_FILESYSPATH, &fileName);
								if (SUCCEEDED(hr))
								{
									CoTaskMemFree(oldFileName);
									SaveWorkingFile();
									SetWindowTextW(hDirEdit, fileName);
									EnableMenuItem(GetMenu(hWndMain), ID_FILE_SAVE, MF_BYCOMMAND | MF_ENABLED);
									haveUnsaved = false;
								}
								else fileName = oldFileName;
								psiResult->Release();
							}
						}
						pfdc->Release();
					}
					//
					// You can add your own code here to handle the results.
					//
				}
			}
			pfd->Release();
		}
		CoUninitialize();
	}
	return hr;
}
