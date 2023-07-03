// ClipboardPekoify.cpp : Defines the entry point for the application.
//
#include "ClipboardPekoify.h"

HWND hwndNextViewer;
NOTIFYICONDATA nid;
static const wchar_t* replace_en = L" peko";
static const wchar_t* replace_jp = L"ぺこ";
// Enabled - enabled
// MangleLinks - everything with no checks (default), dont mangle links (disabled)
bool enabled = true;
bool mangleLinks = true;

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

void ModifyClipboardText()
{
    if (!OpenClipboard(nullptr))
    {
        return;
    }

    // Get the clipboard data handle
    HANDLE hData = GetClipboardData(CF_UNICODETEXT);
    if (hData == nullptr)
    {
        CloseClipboard();
        return;
    }

    // Lock the data to access its content
    LPWSTR pData = static_cast<LPWSTR>(GlobalLock(hData));
    if (pData == nullptr)
    {
        CloseClipboard();
        return;
    }

    // Disable pekoifying for links (strings like http*)
    if (!mangleLinks) {
        if (pData[0] == L'h' && pData[1] == L't' && pData[2] == L't' && pData[3] == L'p') {
            CloseClipboard();
			return;
		}
		else if (pData[0] == L'H' && pData[1] == L'T' && pData[2] == L'T' && pData[3] == L'P') {
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
            shouldModify = false;
            break;
        }
        else if (i <= 3 && ispunct_fullwidth(pData[i]))
        {
            shouldModify = false;
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
            // Warning: update i += x if you change the length of the strings
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
            }
            else
            {
                GlobalFree(hModifiedData);
            }
        }
        else
        {
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
        if (enabled) {
            ModifyClipboardText();
        }

        // Pass the message to the next clipboard viewer in the chain
        SendMessage(hwndNextViewer, uMsg, wParam, lParam);
    }
    if (uMsg == WM_USER + 1) {
    }
	if (uMsg == WM_COMMAND) {
		switch (LOWORD(wParam)) {
		case 800:
			enabled = !enabled;
            break;
		case 801:
			mangleLinks = !mangleLinks;
			break;
		case 802:
			DestroyWindow(hwnd);
            PostQuitMessage(0);
			break;
		}
    }
    // Call the default window procedure for other messages
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

// Note: Update subsystem to use /SUBSYSTEM:CONSOLE and replace WinMain with main if you want a console window
int WINAPI WinMain(HINSTANCE hInst, HINSTANCE hPrevInst, LPSTR, int)
{
    // Check if another instance is already running
    HANDLE hMutex = CreateMutex(nullptr, TRUE, L"ClipboardPekoify");
   if (GetLastError() == ERROR_ALREADY_EXISTS)
	{
		return 0;
	}

    // Create a hidden window to handle clipboard messages
    HINSTANCE hInstance = GetModuleHandle(nullptr);
    WNDCLASSEXW wx = {sizeof(WNDCLASSEXW)};
    wx.lpfnWndProc = ClipboardViewerProc;
	wx.hInstance = hInstance;
    wx.lpszClassName = L"ClipboardPekoify";
    RegisterClassExW(&wx);
    HWND hwnd = CreateWindowEx(WS_EX_TOPMOST, L"ClipboardPekoify", L"ClipboardPekoify", WS_POPUP, 0, 0, 0, 0, HWND_MESSAGE, nullptr, hInstance, nullptr);
    hwndNextViewer = SetClipboardViewer(hwnd);
    SetWindowLongPtr(hwnd, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(ClipboardViewerProc));

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