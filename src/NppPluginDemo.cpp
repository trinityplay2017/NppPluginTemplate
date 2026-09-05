//this file is part of notepad++
//Copyright (C)2022 Don HO <don.h@free.fr>
//
//This program is free software; you can redistribute it and/or
//modify it under the terms of the GNU General Public License
//as published by the Free Software Foundation; either
//version 2 of the License, or (at your option) any later version.
//
//This program is distributed in the hope that it will be useful,
//but WITHOUT ANY WARRANTY; without even the implied warranty of
//MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//GNU General Public License for more details.
//
//You should have received a copy of the GNU General Public License
//along with this program; if not, write to the Free Software
//Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

#include "PluginDefinition.h"
static WNDPROC g_originalNppWndProc = nullptr;
static HWND g_hNppWnd = nullptr;
extern FuncItem funcItem[nbFunc];
extern NppData nppData;
LRESULT CALLBACK NppWndProc(
	HWND hwnd,
	UINT msg,
	WPARAM wParam,
	LPARAM lParam);

BOOL APIENTRY DllMain(HANDLE hModule, DWORD  reasonForCall, LPVOID /*lpReserved*/)
{
	try {

		switch (reasonForCall)
		{
			case DLL_PROCESS_ATTACH:
				pluginInit(hModule);
				break;

			case DLL_PROCESS_DETACH:
				pluginCleanUp();
				break;

			case DLL_THREAD_ATTACH:
				break;

			case DLL_THREAD_DETACH:
				break;
		}
	}
	catch (...) { return FALSE; }

    return TRUE;
}
#include "menuCmdID.h"
HWND GetCurrentScintilla()
{
	int which = -1;

	::SendMessage(
		nppData._nppHandle,
		NPPM_GETCURRENTSCINTILLA,
		0,
		reinterpret_cast<LPARAM>(&which));

	if (which == 0)
		return nppData._scintillaMainHandle;

	if (which == 1)
		return nppData._scintillaSecondHandle;

	return nullptr;
}
#include <filesystem>
std::wstring GetCurrentFileName()
{
	wchar_t buffer[MAX_PATH] = {};

	::SendMessageW(
		nppData._nppHandle,
		NPPM_GETFULLCURRENTPATH,
		MAX_PATH,
		reinterpret_cast<LPARAM>(buffer));

	return std::wstring(buffer);
}
#include "MHFileEx.h"
bool SaveCurrentDocumentUsingMHFileEx()
{
	HWND hwndSci = GetCurrentScintilla();

	if (!hwndSci)
		return false;

	std::wstring fileName = GetCurrentFileName();

	if (fileName.empty())
		return false;

	LRESULT length =
		::SendMessage(
			hwndSci,
			SCI_GETTEXTLENGTH,
			0,
			0);

	if (length < 0)
		return false;

	std::vector<char> buffer(
		static_cast<size_t>(length) + 1);

	::SendMessageA(
		hwndSci,
		SCI_GETTEXT,
		static_cast<WPARAM>(length) + 1,
		reinterpret_cast<LPARAM>(buffer.data()));

	buffer[length] = '\0';

	/*
		MHFileEx expects char*.
	*/

	//CMHFileEx file;

	cMHFile.SetData(buffer.data());

	std::string ansiPath =
		std::filesystem::path(fileName).string();

	if (!cMHFile.SaveToBin(ansiPath.c_str()))
		return false;

	::SendMessage(
		hwndSci,
		SCI_SETSAVEPOINT,
		0,
		0);

	return true;
}
LRESULT CALLBACK NppWndProc(
	HWND hwnd,
	UINT msg,
	WPARAM wParam,
	LPARAM lParam)
{
	if (msg == WM_COMMAND)
	{
		if (LOWORD(wParam) == IDM_FILE_SAVE)
		{
		//	MessageBox(nullptr, L"test", L"", MB_OK);
		//	return 0;
			if (SaveCurrentDocumentUsingMHFileEx())
				return 0;
		}
	}

	return ::CallWindowProcW(
		g_originalNppWndProc,
		hwnd,
		msg,
		wParam,
		lParam);
}
void RemoveNppHook()
{
	if (!g_hNppWnd || !g_originalNppWndProc)
		return;

	::SetWindowLongPtrW(
		g_hNppWnd,
		GWLP_WNDPROC,
		reinterpret_cast<LONG_PTR>(g_originalNppWndProc));

	g_originalNppWndProc = nullptr;
	g_hNppWnd = nullptr;
}
bool InstallNppHook()
{
	if (!nppData._nppHandle)
		return false;

	if (g_originalNppWndProc)
		return true;

	g_hNppWnd = nppData._nppHandle;

	g_originalNppWndProc =
		reinterpret_cast<WNDPROC>(
			::SetWindowLongPtrW(
				g_hNppWnd,
				GWLP_WNDPROC,
				reinterpret_cast<LONG_PTR>(NppWndProc)));

	if (!g_originalNppWndProc)
	{
		g_hNppWnd = nullptr;
		return false;
	}

	return true;
}

extern "C" __declspec(dllexport) void setInfo(NppData notpadPlusData)
{
	nppData = notpadPlusData;
	commandMenuInit();
	InstallNppHook();
}

extern "C" __declspec(dllexport) const TCHAR * getName()
{
	return NPP_PLUGIN_NAME;
}

extern "C" __declspec(dllexport) FuncItem * getFuncsArray(int *nbF)
{
	*nbF = nbFunc;
	return funcItem;
}


extern "C" __declspec(dllexport) void beNotified(SCNotification *notifyCode)
{
	switch (notifyCode->nmhdr.code) 
	{
		case NPPN_SHUTDOWN:
		{
			commandMenuCleanUp();
		}
		break;

		default:
			return;
	}
}


// Here you can process the Npp Messages 
// I will make the messages accessible little by little, according to the need of plugin development.
// Please let me know if you need to access to some messages :
// https://github.com/notepad-plus-plus/notepad-plus-plus/issues
//
extern "C" __declspec(dllexport) LRESULT messageProc(UINT /*Message*/, WPARAM /*wParam*/, LPARAM /*lParam*/)
{/*
	if (Message == WM_MOVE)
	{
		::MessageBox(NULL, "move", "", MB_OK);
	}
*/
	return TRUE;
}

#ifdef UNICODE
extern "C" __declspec(dllexport) BOOL isUnicode()
{
    return TRUE;
}
#endif //UNICODE
