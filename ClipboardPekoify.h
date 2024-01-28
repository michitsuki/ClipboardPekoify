#pragma once
#define UNICODE
#include <iostream>
#include <Windows.h>
#include <ShlObj.h>
#include <shlwapi.h>
#include <shellapi.h>
#include <vector>
#include <winreg.h>
#include "resource.h"
#include "localizable.h"

#define PEKOIFY_VERSION 1

// Function Prototypes
bool ispunct(wchar_t c);
bool ispunct_fullwidth(wchar_t c);
bool isJapaneseText(wchar_t c);
bool isLink(std::wstring text);
void ModifyClipboardText();
bool checkInstalled();
bool writeSettings();
bool readSettings();
bool launchStartup(bool toggle);

// Global Variables
static const wchar_t* replace_en = L" peko";
static const wchar_t* replace_jp = L"ぺこ";
static const std::vector<std::wstring> prefixes{ L"http", L"ftp", L"irc", L"sftp", L"magnet" };