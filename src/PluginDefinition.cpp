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
#include "menuCmdID.h"

#include <windows.h>
#include <shlobj.h>
#include <shlwapi.h>
#include <vector>
#include <string>

FuncItem funcItem[nbFunc];
NppData nppData;

void pluginInit(HANDLE /*hModule*/)
{
}

void pluginCleanUp()
{
}

void commandMenuInit()
{
    setCommand(0, TEXT("Hello Notepad++"), hello, NULL, false);
    setCommand(1, TEXT("Hello (with dialog) test"), helloDlg, NULL, false);
    setCommand(2, TEXT("Load TXT Files from Folder..."), loadTxtFilesFromFolder, NULL, false);
}

void commandMenuCleanUp()
{
}

bool setCommand(size_t index, TCHAR *cmdName, PFUNCPLUGINCMD pFunc, ShortcutKey *sk, bool check0nInit)
{
    if (index >= nbFunc)
        return false;

    if (!pFunc)
        return false;

    lstrcpy(funcItem[index]._itemName, cmdName);
    funcItem[index]._pFunc = pFunc;
    funcItem[index]._init2Check = check0nInit;
    funcItem[index]._pShKey = sk;

    return true;
}

void hello()
{
    ::SendMessage(nppData._nppHandle, NPPM_MENUCOMMAND, 0, IDM_FILE_NEW);

    int which = -1;
    ::SendMessage(nppData._nppHandle, NPPM_GETCURRENTSCINTILLA, 0, (LPARAM)&which);
    if (which == -1)
        return;

    HWND curScintilla = (which == 0) ? nppData._scintillaMainHandle : nppData._scintillaSecondHandle;

    ::SendMessage(curScintilla, SCI_SETTEXT, 0, (LPARAM)"Hello, Notepad++!");
}

void helloDlg()
{
    ::MessageBox(NULL, TEXT("Hello, Notepad++!"), TEXT("Notepad++ Plugin Template"), MB_OK);
}
#include <shobjidl.h>
#include <atlbase.h>
#include <fstream>
#include <vector>
static std::wstring selectFolderBAK()
{
    BROWSEINFOW bi = {};
    bi.hwndOwner = nppData._nppHandle;
    bi.lpszTitle = L"Select a folder containing TXT files";
    bi.ulFlags = BIF_RETURNONLYFSDIRS | BIF_NEWDIALOGSTYLE;

    PIDLIST_ABSOLUTE pidl = ::SHBrowseForFolderW(&bi);
    if (!pidl)
        return std::wstring();

    wchar_t path[MAX_PATH] = {};
    std::wstring result;

    if (::SHGetPathFromIDListW(pidl, path))
        result = path;

    ::CoTaskMemFree(pidl);

    return result;
}
#include <atlstr.h>
void SetInitialFolder(IUnknown* dialog, const std::wstring& initialFolder)
{
    if (!initialFolder.empty())
        return;

    CComPtr<IShellItem> psiFolder;
    const wchar_t* buff = initialFolder.c_str();
    if (SUCCEEDED(SHCreateItemFromParsingName(buff, nullptr, IID_PPV_ARGS(&psiFolder))))
    {
        CComPtr<IFileDialog> pFileDialog;
        if (SUCCEEDED(dialog->QueryInterface(&pFileDialog)))
        {
            pFileDialog->SetFolder(psiFolder);
        }
    }
}
BOOL GetSingleResult(IFileDialog* dlg, CStringW& outPath)
{
    CComPtr<IShellItem> item;
    if (FAILED(dlg->GetResult(&item)))
        return FALSE;

    PWSTR psz = nullptr;
    if (FAILED(item->GetDisplayName(SIGDN_FILESYSPATH, &psz)))
        return FALSE;

    outPath = psz;
    CoTaskMemFree(psz);
    return TRUE;
}
BOOL SelectFolder(HWND hParent, CStringW& outPath, const wchar_t* initialFolder)
{
    CComPtr<IFileOpenDialog> dlg;
    if (FAILED(dlg.CoCreateInstance(__uuidof(FileOpenDialog))))
        return FALSE;

    DWORD opts = 0;
    if (FAILED(dlg->GetOptions(&opts)))
        return FALSE;

    opts |= FOS_PICKFOLDERS | FOS_FORCEFILESYSTEM;
    dlg->SetOptions(opts);

    SetInitialFolder(dlg, initialFolder);

    if (FAILED(dlg->Show(hParent)))
        return FALSE;

    return GetSingleResult(dlg, outPath);
}


static void collectTxtFiles(const std::wstring& folder, std::vector<std::wstring>& files)
{
    std::vector<std::wstring> folders;
    folders.push_back(folder);

    while (!folders.empty())
    {
        std::wstring current = folders.back();
        folders.pop_back();

        std::wstring pattern = current;
        if (!pattern.empty() && pattern.back() != L'\\')
            pattern += L'\\';
        pattern += L'*';

        WIN32_FIND_DATAW fd = {};
        HANDLE hFind = ::FindFirstFileW(pattern.c_str(), &fd);
        if (hFind == INVALID_HANDLE_VALUE)
            continue;

        do
        {
            if (!lstrcmpW(fd.cFileName, L".") || !lstrcmpW(fd.cFileName, L".."))
                continue;

            std::wstring fullPath = current;
            if (!fullPath.empty() && fullPath.back() != L'\\')
                fullPath += L'\\';
            fullPath += fd.cFileName;

            if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
            {
                if (!(fd.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT))
                    folders.push_back(fullPath);
                continue;
            }

            const wchar_t* extension = ::PathFindExtensionW(fd.cFileName);
            if (extension && !lstrcmpiW(extension, L".txt"))
                files.push_back(fullPath);
        }
        while (::FindNextFileW(hFind, &fd));

        ::FindClose(hFind);
    }
}
static bool loadFileAsCharBuffer(const std::wstring& filePath, std::vector<char>& buffer)
{
    std::ifstream file(filePath, std::ios::binary | std::ios::ate);

    if (!file)
        return false;

    std::streamsize size = file.tellg();

    if (size < 0)
        return false;

    file.seekg(0, std::ios::beg);

    buffer.resize(static_cast<size_t>(size) + 1);

    if (size > 0)
    {
        if (!file.read(buffer.data(), size))
            return false;
    }

    buffer[static_cast<size_t>(size)] = '\0';

    return true;
}
void loadTxtFilesFromFolder()
{
   // CStringW folder;

    /*if (!SelectFolder(
        nppData._nppHandle,
        std::wstring(),
        folder))
    {
        return;
    }*/
    CStringW folder;
    SelectFolder(NULL, folder, L"*.*");

    std::vector<std::wstring> files;
    collectTxtFiles(folder.GetString(), files);

    int which = -1;

    ::SendMessage(
        nppData._nppHandle,
        NPPM_GETCURRENTSCINTILLA,
        0,
        reinterpret_cast<LPARAM>(&which));

    if (which == -1)
        return;

    HWND curScintilla =
        (which == 0)
        ? nppData._scintillaMainHandle
        : nppData._scintillaSecondHandle;

    for (const std::wstring& file : files)
    {
        std::vector<char> buffer;

        if (!loadFileAsCharBuffer(file, buffer))
            continue;

        ::SendMessageW(nppData._nppHandle, NPPM_DOOPEN, 0, (LPARAM)file.c_str());

        ::SendMessageA(
            curScintilla,
            SCI_SETTEXT,
            0,
            reinterpret_cast<LPARAM>(buffer.data()));
    }

    if (files.empty())
    {
        ::MessageBoxW(
            nppData._nppHandle,
            L"No .txt files were found in the selected folder.",
            L"Load TXT Files",
            MB_OK | MB_ICONINFORMATION);
    }
}
void loadTxtFilesFromFolderBAK()
{
    CStringW folder;
    SelectFolder(NULL, folder, L"*.*");
    
    if (folder.IsEmpty())
        return;

    std::vector<std::wstring> files;
    collectTxtFiles(folder.GetString(), files);

    for (const std::wstring& file : files)
        ::SendMessageW(nppData._nppHandle, NPPM_DOOPEN, 0, (LPARAM)file.c_str());

    if (files.empty())
    {
        ::MessageBoxW(
            nppData._nppHandle,
            L"No .txt files were found in the selected folder.",
            L"Load TXT Files",
            MB_OK | MB_ICONINFORMATION);
    }
}
