// ClipboardPekoify.cpp : Defines the entry point for the application.
//
#include "ClipboardPekoify.h"

HWND hwndNextViewer;
NOTIFYICONDATA nid;
// MangleLinks - everything with no checks (default), dont mangle links (disabled)
bool enabled = true;
bool mangleLinks = true;
bool installed = false;

// Override so only punctuation that is generally seen at the end of a sentence is replaced
bool ispunct(wchar_t c)
{
	return c == L'.' || c == L'!' || c == L'?' || c == L'~';
}

bool ispunct_fullwidth(wchar_t c) {
	return c == L'。' || c == L'！' || c == L'？' || c == L'～';
}

bool isJapaneseText(wchar_t c) {
	return c >= 0x3040 && c <= 0x30ff;
}

bool isLink(std::wstring text) {
	for (size_t i = 0; i < prefixes.size(); i++) {
		if (prefixes[i].length() > text.length()) continue;
		if (wcsncmp(text.c_str(), prefixes[i].c_str(), prefixes[i].length()) == 0) {
			return true;
		}
	}
	return false;
}

// Checks if it launches on startup, if toggle is true it will toggle the setting
// Checks ClipboardPekoify in HKCU\Software\Microsoft\Windows\CurrentVersion\Run
bool launchStartup(bool toggle) {
	if (toggle) {
		if (launchStartup(false)) {
			// Delete the key
			HKEY hKey;
			LONG lRes = RegOpenKeyExW(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Run", 0, KEY_WRITE, &hKey);
			if (lRes != ERROR_SUCCESS) {
				MessageBoxExW(NULL, LOC_UNKNOWNREGERR, LOC_PEKOIFY, MB_OK | MB_ICONERROR, 0);
				return false;
			}
			lRes = RegDeleteValueW(hKey, L"ClipboardPekoify");
			if (lRes != ERROR_SUCCESS) {
				MessageBoxExW(NULL, LOC_SETTINGSAVEFAILED, LOC_PEKOIFY, MB_OK | MB_ICONERROR, 0);
				return false;
			}
			RegCloseKey(hKey);
		}
		else {
			// Create the key
			HKEY hKey;
			LONG lRes = RegOpenKeyExW(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Run", 0, KEY_WRITE, &hKey);
			if (lRes != ERROR_SUCCESS) {
				MessageBoxExW(NULL, LOC_UNKNOWNREGERR, LOC_PEKOIFY, MB_OK | MB_ICONERROR, 0);
				return false;
			}
			wchar_t path[MAX_PATH];
			if (GetModuleFileNameW(NULL, path, MAX_PATH) == 0) {
				MessageBoxExW(NULL, LOC_UNKNOWNERR, LOC_PEKOIFY, MB_OK | MB_ICONERROR, 0);
				return false;
			}
			lRes = RegSetValueExW(hKey, L"ClipboardPekoify", 0, REG_SZ, reinterpret_cast<BYTE*>(path), (wcslen(path) + 1) * sizeof(wchar_t));
			if (lRes != ERROR_SUCCESS) {
				MessageBoxExW(NULL, LOC_SETTINGSAVEFAILED, LOC_PEKOIFY, MB_OK | MB_ICONERROR, 0);
				return false;
			}
			RegCloseKey(hKey);
			return true;
		}
	}
	else {
		HKEY hKey;
		LONG lRes = RegOpenKeyExW(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Run", 0, KEY_READ, &hKey);
		if (lRes != ERROR_SUCCESS) {
			MessageBoxExW(NULL, LOC_UNKNOWNREGERR, LOC_PEKOIFY, MB_OK | MB_ICONERROR, 0);
			return false;
		}
		DWORD dwType = REG_SZ;
		DWORD dwSize = 255;
		wchar_t path[255];
		lRes = RegQueryValueExW(hKey, L"ClipboardPekoify", NULL, &dwType, reinterpret_cast<BYTE*>(path), &dwSize);
		if (lRes != ERROR_SUCCESS) {
			return false;
		}
		RegCloseKey(hKey);
		if (wcscmp(path, L"") == 0) {
			return false;
		}
		return true;
	}
	return false;
}

bool checkInstalled() {
	HKEY hKey;
	LONG lRes = RegOpenKeyExW(HKEY_CURRENT_USER, L"Software\\ClipboardPekoify", 0, KEY_READ, &hKey);
	if (lRes != ERROR_SUCCESS) {
		return false;
	}
	// Check version, if running > registry, return false to update
	DWORD dwType = REG_DWORD;
	DWORD dwSize = sizeof(DWORD);
	DWORD version;
	lRes = RegQueryValueExW(hKey, L"Version", NULL, &dwType, reinterpret_cast<BYTE*>(&version), &dwSize);
	if (lRes != ERROR_SUCCESS) {
		return false;
	}
	if (version < PEKOIFY_VERSION) {
		return false;
	}
	RegCloseKey(hKey);
	return true;
}

// Writes MangleLinks and Enabled to the registry
bool writeSettings() {
	HKEY hKey;
	LONG lRes = RegOpenKeyExW(HKEY_CURRENT_USER, L"Software\\ClipboardPekoify", 0, KEY_WRITE, &hKey);
	if (lRes != ERROR_SUCCESS) {
		return false;
	}
	lRes = RegSetValueExW(hKey, L"MangleLinks", 0, REG_DWORD, reinterpret_cast<BYTE*>(&mangleLinks), sizeof(DWORD));
	if (lRes != ERROR_SUCCESS) {
		return false;
	}
	lRes = RegSetValueExW(hKey, L"Enabled", 0, REG_DWORD, reinterpret_cast<BYTE*>(&enabled), sizeof(DWORD));
	if (lRes != ERROR_SUCCESS) {
		return false;
	}
	BYTE version = PEKOIFY_VERSION;
	lRes = RegSetValueExW(hKey, L"Version", 0, REG_DWORD, reinterpret_cast<BYTE*>(&version), sizeof(DWORD));
	if (lRes != ERROR_SUCCESS) {
		return false;
	}

	RegCloseKey(hKey);
	return true;
}

// Reads MangleLinks and Enabled from the registry, if they dont exist they are set to true
// After installation, they will not exist and will be set when the user makes a change
bool readSettings() {
	HKEY hKey;
	LONG lRes = RegOpenKeyExW(HKEY_CURRENT_USER, L"Software\\ClipboardPekoify", 0, KEY_READ, &hKey);
	if (lRes != ERROR_SUCCESS) {
		return false;
	}
	DWORD dwType = REG_DWORD;
	DWORD dwSize = sizeof(DWORD);
	lRes = RegQueryValueExW(hKey, L"MangleLinks", NULL, &dwType, reinterpret_cast<BYTE*>(&mangleLinks), &dwSize);
	if (lRes != ERROR_SUCCESS) {
		mangleLinks = true;
	}
	lRes = RegQueryValueExW(hKey, L"Enabled", NULL, &dwType, reinterpret_cast<BYTE*>(&enabled), &dwSize);
	if (lRes != ERROR_SUCCESS) {
		enabled = true;
	}
	RegCloseKey(hKey);
	return true;
}

void ModifyClipboardText()
{
	if (!OpenClipboard(nullptr))
	{
#ifdef PRINTCONSOLE
		std::cout << "Failed to open the clipboard." << std::endl;
#endif
		return;
	}

	// Get the clipboard data handle
	HANDLE hData = GetClipboardData(CF_UNICODETEXT);
	if (hData == nullptr)
	{
#ifdef PRINTCONSOLE
		std::cout << "Failed to retrieve clipboard data." << std::endl;
#endif
		CloseClipboard();
		return;
	}

	// Lock the data to access its content
	LPWSTR pData = static_cast<LPWSTR>(GlobalLock(hData));
	if (pData == nullptr)
	{
#ifdef PRINTCONSOLE
		std::cout << "Failed to lock clipboard data." << std::endl;
#endif
		CloseClipboard();
		return;
	}

	// Disable pekoifying for links (strings like http*)
	if (!mangleLinks) {
		if (isLink(pData)) {
#ifdef PRINTCONSOLE
			std::cout << "Clipboard text is a link." << std::endl;
#endif
			CloseClipboard();
			return;
		}
	}


	// Check if the text has already been modified
	bool shouldModify = true;
	for (size_t i = 0; pData[i] != L'\0'; i++)
	{
		if (i <= 5 && ispunct(pData[i]))
		{
			break;
		}
		else if (i <= 3 && ispunct_fullwidth(pData[i]))
		{
			break;
		}
		else
		{
			if (i > 5 && ispunct(pData[i]) && pData[i - 5] == L' ' && pData[i - 4] == L'p' && pData[i - 3] == L'e' && pData[i - 2] == L'k' && pData[i - 1] == L'o')
			{
				shouldModify = false;
				break;
}
			else if (i > 3 && ispunct_fullwidth(pData[i]) && pData[i - 2] == L'ぺ' && pData[i - 1] == L'こ')
			{
				shouldModify = false;
				break;
			}
		}
	}

	if (shouldModify)
	{
		// Convert the clipboard text to a std::wstring
		std::wstring modifiedText(pData);
		for (size_t i = 0; i < modifiedText.length(); i++)
		{
			// Catch mixed halfwidth punctuation in Japanese text
			if(ispunct(modifiedText[i]) && i > 0 && isJapaneseText(modifiedText[i - 1])) {
				modifiedText.insert(i, replace_jp);
				i += 2;
				continue;
			}
			if (ispunct(modifiedText[i]))
			{
				modifiedText.insert(i, replace_en);
				i += 5; // Skip past the inserted " peko"
			}
			else if (ispunct_fullwidth(modifiedText[i]))
			{
				modifiedText.insert(i, replace_jp);
				i += 2; // Skip past the inserted "ぺこ"
			}
		}

		HGLOBAL hModifiedData = GlobalAlloc(GMEM_MOVEABLE, (modifiedText.length() + 1) * sizeof(wchar_t));
		if (hModifiedData != nullptr)
		{
			// Lock the memory and copy the modified text
			LPWSTR pModifiedData = static_cast<LPWSTR>(GlobalLock(hModifiedData));
			if (pModifiedData != nullptr)
			{
				wcscpy_s(pModifiedData, modifiedText.length() + 1, modifiedText.c_str());
				GlobalUnlock(hModifiedData);

				EmptyClipboard();
				SetClipboardData(CF_UNICODETEXT, hModifiedData);
#ifdef PRINTCONSOLE
				std::cout << "Clipboard text modified successfully!" << std::endl;
#endif
			}
			else
			{
#ifdef PRINTCONSOLE
				std::cout << "Failed to lock the memory for modified text." << std::endl;
#endif
				GlobalFree(hModifiedData);
			}
		}
		else
		{
#ifdef PRINTCONSOLE
			std::cout << "Failed to allocate memory for modified text." << std::endl;
#endif
		}
	}

	// Cleanup
	GlobalUnlock(hData);
	CloseClipboard();
}


// Message handler function
LRESULT CALLBACK ClipboardViewerProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	if (uMsg == WM_DRAWCLIPBOARD)
	{
#ifdef PRINTCONSOLE
		std::cout << "Clipboard content changed." << std::endl;
#endif
		
		if (enabled) {
			ModifyClipboardText();
		}

		// Pass the message to the next clipboard viewer in the chain
		SendMessage(hwndNextViewer, uMsg, wParam, lParam);
	}
	if (uMsg == WM_USER + 1) {
		if (LOWORD(lParam) == WM_RBUTTONUP || LOWORD(lParam) == WM_LBUTTONUP) {
			POINT lpClickPoint;
			GetCursorPos(&lpClickPoint);
			HMENU hPopMenu = CreatePopupMenu();
			AppendMenuW(hPopMenu, MF_STRING | MF_GRAYED, 900, LOC_PEKOIFYHEADER);
			AppendMenuW(hPopMenu, MF_STRING, 804, KONPEKO);
			AppendMenuW(hPopMenu, MF_SEPARATOR, 901, L"");
			AppendMenuW(hPopMenu, MF_STRING, 800, enabled ? LOC_DISABLE : LOC_ENABLE);
			AppendMenuW(hPopMenu, MF_STRING | (!mangleLinks ? MF_CHECKED : MF_UNCHECKED), 801, LOC_IGNORELINKS);
			AppendMenuW(hPopMenu, MF_STRING | (launchStartup(false) ? MF_CHECKED : MF_UNCHECKED), 803, LOC_LAUNCHSTARTUP);
			AppendMenuW(hPopMenu, MF_SEPARATOR, 901, L"");
			AppendMenuW(hPopMenu, MF_STRING, 802, LOC_EXIT);
			SetForegroundWindow(hwnd);
			TrackPopupMenu(hPopMenu, TPM_LEFTALIGN | TPM_LEFTBUTTON | TPM_BOTTOMALIGN, lpClickPoint.x, lpClickPoint.y, 0, hwnd, NULL);
			PostMessageW(hwnd, WM_NULL, 0, 0);
		}
	}
	if (uMsg == WM_COMMAND) {
		switch (LOWORD(wParam)) {
		case 800:
			enabled = !enabled;
			if (enabled && mangleLinks) {
				wcscpy_s(nid.szTip, LOC_ENABLEDMANGLE);
			}
			else if (enabled && !mangleLinks) {
				wcscpy_s(nid.szTip, LOC_ENABLEDNOMANGLE);
			}
			else {
				wcscpy_s(nid.szTip, LOC_DISABLED);
			}
			if(installed) {
				if (!writeSettings()) {
					MessageBoxExW(NULL, LOC_SETTINGSAVEFAILED, LOC_PEKOIFY, MB_OK | MB_ICONINFORMATION, 0);
				}
			}
			break;
		case 801:
			mangleLinks = !mangleLinks;
			if (enabled && mangleLinks) {
				wcscpy_s(nid.szTip, LOC_ENABLEDMANGLE);
			}
			else if (enabled && !mangleLinks) {
				wcscpy_s(nid.szTip, LOC_ENABLEDNOMANGLE);
			}
			else {
				wcscpy_s(nid.szTip, LOC_DISABLED);
			}
			if (installed) {
				if (!writeSettings()) {
					MessageBoxExW(NULL, LOC_SETTINGSAVEFAILED, LOC_PEKOIFY, MB_OK | MB_ICONINFORMATION, 0);
				}
			}
			break;
		case 802:
			DestroyWindow(hwnd);
			PostQuitMessage(0);
			break;
		case 803:
			if (!installed) {
				MessageBoxExW(NULL, LOC_NOTINSTALLED, LOC_PEKOIFY, MB_OK | MB_ICONINFORMATION, 0);
				break;
			}
			launchStartup(true);
			break;
		case 804:
			HANDLE hToken;
			TOKEN_PRIVILEGES tkp;
			if (!OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken)) {
#ifdef PRINTCONSOLE
				std::cout << "Failed to open process token for 804." << std::endl;
#endif
				break;
			}
			LookupPrivilegeValueW(NULL, SE_SHUTDOWN_NAME, &tkp.Privileges[0].Luid);
			tkp.PrivilegeCount = 1;
			tkp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
			if (!AdjustTokenPrivileges(hToken, FALSE, &tkp, 0, (PTOKEN_PRIVILEGES)NULL, 0)) {
#ifdef PRINTCONSOLE
				std::cout << "Failed to adjust token privileges for 804." << std::endl;
#endif
				break;
			}
			LPWSTR pText = new wchar_t[255];
			wcscpy_s(pText, 255, LOC_PEKOINTRO);
			int ret = InitiateSystemShutdownExW(NULL,pText, 0, TRUE, FALSE, SHTDN_REASON_MAJOR_OPERATINGSYSTEM | SHTDN_REASON_MINOR_SECURITY);
#ifdef PRINTCONSOLE
			if (ret == 0) {
				std::cout << "Shutdown failed. Error code: ";
				std::cout << GetLastError() << std::endl;
			}
#endif
			break;
		}
	}
	// Call the default window procedure for other messages
	return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

// Note: Update subsystem to use /SUBSYSTEM:CONSOLE and define PRINTCONSOLE to enable extra output
#ifdef PRINTCONSOLE
int WINAPI main(HINSTANCE hInst, HINSTANCE hPrevInst, LPSTR, int)
#else
int WINAPI WinMain(HINSTANCE hInst, HINSTANCE hPrevInst, LPSTR, int)
#endif
{
	// Check if another instance is already running
	HANDLE hMutex = CreateMutex(nullptr, TRUE, LOC_PEKOIFY);
   if (GetLastError() == ERROR_ALREADY_EXISTS)
	{
		MessageBoxExW(NULL,LOC_EXISTINGINST, LOC_PEKOIFY, MB_OK | MB_ICONERROR, 0);
		return 0;
	}

   if (!checkInstalled()) {
	   int ret = MessageBoxExW(NULL, LOC_ASKINSTALL, LOC_PEKOIFY, MB_YESNO | MB_ICONQUESTION, 0);
	   if (ret == IDYES) {
		   // Copy executable to LocalAppData\ClipboardPekoify and restart from there
		   wchar_t path[MAX_PATH];
		   if (SHGetFolderPathW(NULL, CSIDL_LOCAL_APPDATA, NULL, 0, path) != S_OK) {
			   MessageBoxExW(NULL, LOC_INSTERRCOPYFILE, LOC_PEKOIFY, MB_OK | MB_ICONERROR, 0);
			   return 1;
		   }
		   // Create directory
		   wcscat_s(path, L"\\ClipboardPekoify");
		   if (!CreateDirectoryW(path, NULL)) {
			   if (GetLastError() != ERROR_ALREADY_EXISTS) {
				   MessageBoxExW(NULL, LOC_INSTERRCOPYFILE, LOC_PEKOIFY, MB_OK | MB_ICONERROR, 0);
				   return 1;
			   }
		   }
		   wcscat_s(path, L"\\ClipboardPekoify.exe");
		   LPWSTR a = GetCommandLineW();
		   if (a == nullptr || wcslen(a) < 1) {
			   MessageBoxExW(NULL, LOC_INSTFATAL, LOC_PEKOIFY, MB_OK | MB_ICONERROR, 0);
			   return 1;
		   }
		   if (a[0] == L'"') {
			   size_t len = wcslen(a);
			   memmove(a, a + 1, (len - 1) * sizeof(wchar_t));
			   for (int i = 1; i < len; i++) {
				   if (a[i] == L'"') {
					   a[i] = L'\0';
					   break;
				   }
			   }
		   }

		   if (!CopyFileW(a, path, FALSE)) {
			   DWORD a = GetLastError();
			   MessageBoxExW(NULL, LOC_INSTERRCOPYFILE, LOC_PEKOIFY, MB_OK | MB_ICONERROR, 0);
			   return 1;
		   }
		   // Create start menu shortcut
		   wchar_t shortcutPath[MAX_PATH];
		   if (SHGetFolderPathW(NULL, CSIDL_PROGRAMS, NULL, 0, shortcutPath) != S_OK) {
			   MessageBoxExW(NULL, LOC_INSTMENU, LOC_PEKOIFY, MB_OK | MB_ICONERROR, 0);
			   return 1;
		   }
		   wcscat_s(shortcutPath, L"\\ClipboardPekoify.lnk");
		   IShellLinkW* psl;
		   HRESULT hres = CoInitialize(NULL);
			hres = CoCreateInstance(CLSID_ShellLink, NULL, CLSCTX_INPROC_SERVER, IID_IShellLinkW, reinterpret_cast<void**>(&psl));
		   if (SUCCEEDED(hres)) {
			   psl->SetPath(path);
			   IPersistFile* ppf;
			   hres = psl->QueryInterface(IID_IPersistFile, reinterpret_cast<void**>(&ppf));
			   if (SUCCEEDED(hres)) {
				   hres = ppf->Save(shortcutPath, TRUE);
				   ppf->Release();
			   }
			   psl->Release();
		   }
		   if (hres != S_OK) {
			   MessageBoxExW(NULL, LOC_INSTMENU, LOC_PEKOIFY, MB_OK | MB_ICONERROR, 0);
			   return 1;
		   }
		   // Create registry key
		   HKEY hKey;
		   LONG lRes = RegCreateKeyExW(HKEY_CURRENT_USER, L"Software\\ClipboardPekoify", 0, NULL, REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &hKey, NULL);
		   if (lRes != ERROR_SUCCESS) {
			   MessageBoxExW(NULL, LOC_INSTREG, LOC_PEKOIFY, MB_OK | MB_ICONERROR, 0);
			   return 1;
		   }
		   // set version
		   BYTE version = PEKOIFY_VERSION;
		   lRes = RegSetValueExW(hKey, L"Version", 0, REG_DWORD, reinterpret_cast<BYTE*>(&version), sizeof(DWORD));
		   if (lRes != ERROR_SUCCESS) {
			   MessageBoxExW(NULL, LOC_INSTREG, LOC_PEKOIFY, MB_OK | MB_ICONERROR, 0);
			   return 1;
		   }
		   RegCloseKey(hKey);
		   // Run on startup by default (create registry key)
		   lRes = RegCreateKeyExW(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Run", 0, NULL, REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &hKey, NULL);
		   if (lRes != ERROR_SUCCESS) {
			   MessageBoxExW(NULL, LOC_INSTREG, LOC_PEKOIFY, MB_OK | MB_ICONERROR, 0);
			   return 1;
		   }
		   if (GetModuleFileNameW(NULL, path, MAX_PATH) == 0) {
			   MessageBoxExW(NULL, LOC_INSTFATAL, LOC_PEKOIFY, MB_OK | MB_ICONERROR, 0);
			   return 1;
		   }
		   lRes = RegSetValueExW(hKey, LOC_PEKOIFY, 0, REG_SZ, reinterpret_cast<BYTE*>(path), (wcslen(path) + 1) * sizeof(wchar_t));
		   if (lRes != ERROR_SUCCESS) {
			   MessageBoxExW(NULL, LOC_INSTREG, LOC_PEKOIFY, MB_OK | MB_ICONERROR, 0);
			   return 1;
		   }
		   RegCloseKey(hKey);
		   MessageBoxExW(NULL, LOC_INSTSUCCESS, LOC_PEKOIFY, MB_OK | MB_ICONINFORMATION, 0);
		   // Destroy the mutex which will allow the new instance to run
		   CloseHandle(hMutex);
		   // Quit the current instance and restart from LocalAppData
		   ShellExecuteW(NULL, L"open", path, NULL, NULL, SW_SHOW);
		   return 0;
	   }
	   // If the user does not want to install, we 
	   // continue execution without setting installed to true, which will skip any read and writes to the registry
   }
   else {
	   installed = true;
	   if (!readSettings()) {
		   MessageBoxExW(NULL, LOC_SETTINGLOADFAILED, LOC_PEKOIFY, MB_OK | MB_ICONERROR, 0);
		   enabled = true;
		   mangleLinks = true;
	   }
   }

	// Create a hidden window to handle clipboard messages
	HINSTANCE hInstance = GetModuleHandle(nullptr);
	WNDCLASSEXW wx = {sizeof(WNDCLASSEXW)};
	wx.lpfnWndProc = ClipboardViewerProc;
	wx.hInstance = hInstance;
	wx.lpszClassName = LOC_PEKOIFY;
	RegisterClassExW(&wx);
	HWND hwnd = CreateWindowEx(WS_EX_TOPMOST, LOC_PEKOIFY, LOC_PEKOIFY, WS_POPUP, 0, 0, 0, 0, HWND_MESSAGE, nullptr, hInstance, nullptr);
	hwndNextViewer = SetClipboardViewer(hwnd);
	SetWindowLongPtr(hwnd, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(ClipboardViewerProc));

	// Create tray icon
	nid.cbSize = sizeof(NOTIFYICONDATA);
	nid.hWnd = hwnd;
	nid.uID = 1;
	nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
	nid.uCallbackMessage = WM_USER + 1;
	nid.hIcon = LoadIcon(hInst, MAKEINTRESOURCEW(IDI_ICON1));
	wcscpy_s(nid.szTip, LOC_PEKOIFY);
	Shell_NotifyIcon(NIM_ADD, &nid);
	if(!installed){
		// notify user if not installed, otherwise it saves settings and theres no need to notify
		MessageBoxExW(NULL, LOC_BACKGROUNDRMD, LOC_PEKOIFY, MB_OK | MB_ICONINFORMATION, 0);
	}
	// Enter the message loop
	MSG msg;
	while (GetMessage(&msg, nullptr, 0, 0))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	// Cleanup
	ChangeClipboardChain(hwnd, hwndNextViewer);
	DestroyWindow(hwnd);

	return 0;
}