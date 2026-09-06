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
#define NPPM_INTERNAL_STOPMONITORING (WM_USER + 49)
extern int RefreshT();
#include <atlstr.h>
bool SaveCurrentDocumentUsingMHFileExBUG()
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


	//CStringW folder;

	

	std::vector<std::wstring> files;
	files.push_back(fileName);
	for (const std::wstring& file : files)
	{
		std::vector<char> buffer;
		std::string fullFileName;
		extern bool loadFileAsCharBuffer(
			const std::wstring & filePath,
			std::vector<char>&buffer,
			std::string & fullFileName);
		if (!loadFileAsCharBuffer(
			file,
			buffer,
			fullFileName))
		{
			continue;
		}

		//
		// Let Notepad++ create a normal file buffer.
		//
		::SendMessageW(
			nppData._nppHandle,
			NPPM_DOOPEN,
			0,
			reinterpret_cast<LPARAM>(file.c_str()));

		LRESULT bufferID =
			::SendMessage(
				nppData._nppHandle,
				NPPM_GETCURRENTBUFFERID,
				0,
				0);
		if (bufferID)
		{
			::SendMessage(
				nppData._nppHandle,
				NPPM_INTERNAL_STOPMONITORING,
				static_cast<WPARAM>(bufferID),
				0);
		}
		//
		// Get the newly opened document.
		//
		HWND curScintilla = GetCurrentScintilla();

		if (!curScintilla)
			continue;

		//
		// Replace the contents with CMHFileEx data.
		//
		::SendMessageA(
			curScintilla,
			SCI_SETTEXT,
			0,
			reinterpret_cast<LPARAM>(buffer.data()));

		//
		// Make the document appear unmodified.
		//
		::SendMessage(
			curScintilla,
			SCI_SETSAVEPOINT,
			0,
			0);




		RefreshT();
	}
//	::SendMessage(
//		hwndSci,
//		SCI_SETSAVEPOINT,
//		0,
//		0);

	return true;
}


bool SaveCurrentDocumentUsingMHFileEx()
{
	HWND hwndSci = GetCurrentScintilla();

	if (!hwndSci)
		return false;

	std::wstring fileName =
		GetCurrentFileName();

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

	buffer[static_cast<size_t>(length)] = '\0';

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
#define NOTEPADPLUS_USER_INTERNAL     (WM_USER + 0000)
#define NPPM_INTERNAL_RELOADSCROLLTOEND			    (NOTEPADPLUS_USER_INTERNAL + 42)  // Used by Monitoring feature
static bool IsMHEncryptedFile(const std::wstring& fileName)
{
	std::filesystem::path path(fileName);

	std::wstring ext = path.extension().wstring();

	std::transform(
		ext.begin(),
		ext.end(),
		ext.begin(),
		[](wchar_t c)
		{
			return static_cast<wchar_t>(::towlower(c));
		});

	return
		ext == L".bin" ||
		ext == L".beff" ||
		ext == L".befl" ||
		ext == L".bmhm" ||
		ext == L".bsad";
}
static std::wstring GetFileNameFromBufferID(
	WPARAM bufferID)
{
	if (!bufferID)
		return L"";

	LRESULT length =
		::SendMessage(
			nppData._nppHandle,
			NPPM_GETFULLPATHFROMBUFFERID,
			bufferID,
			0);

	if (length <= 0)
		return L"";

	std::vector<wchar_t> buffer(
		static_cast<size_t>(length) + 1);

	LRESULT result =
		::SendMessageW(
			nppData._nppHandle,
			NPPM_GETFULLPATHFROMBUFFERID,
			bufferID,
			reinterpret_cast<LPARAM>(buffer.data()));

	if (result <= 0)
		return L"";

	return std::wstring(buffer.data());
}

LRESULT CALLBACK NppWndProc(
	HWND hwnd,
	UINT msg,
	WPARAM wParam,
	LPARAM lParam)
{
	/*if (msg >= WM_USER && msg < WM_USER + 100)
	{
		wchar_t text[128]{};
		swprintf_s(
			text,
			L"WM_USER message = 0x%X",
			msg);
		MessageBoxW(
			nullptr,
			text,
			L"DEBUG",
			MB_OK);
	}*/
	if (msg == NPPM_INTERNAL_RELOADSCROLLTOEND)
	{
		//Buffer* buf =
		//	reinterpret_cast<Buffer*>(wParam);

		if (wParam)
		{
			std::wstring fileName =
				GetCurrentFileName();
			MessageBox(nullptr, L"triggered", L"", MB_OK); // not reached
			if (IsMHEncryptedFile(fileName))
			{
				MessageBox(nullptr, L"triggered", L"", MB_OK); // not reached
				//
				// Swallow the reload.
				//
				return 0;
			}
		}
	}

	if (msg == WM_COMMAND)
	{
		if (LOWORD(wParam) == IDM_FILE_SAVE)
		{
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
