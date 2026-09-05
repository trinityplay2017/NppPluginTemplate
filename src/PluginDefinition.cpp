#include "PluginDefinition.h"
#include "menuCmdID.h"
#include <windows.h>
#include <shlobj.h>
#include <shlwapi.h>
#include <vector>
#include <string>
FuncItem funcItem[nbFunc];
NppData nppData;
extern void RemoveNppHook();
void pluginInit(HANDLE )
{
}

void pluginCleanUp()
{
    RemoveNppHook();
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
            if (extension && (
                !lstrcmpiW(extension, L".bin")||
                !lstrcmpiW(extension, L".beff")||
                !lstrcmpiW(extension, L".befl")||
                !lstrcmpiW(extension, L".bmhm")||
                !lstrcmpiW(extension, L".bsad"))
                )
                files.push_back(fullPath);
        }
        while (::FindNextFileW(hFind, &fd));
        ::FindClose(hFind);
    }
}
#include <filesystem>
#include <string>
#include <vector>
static bool loadFileAsCharBuffer(
    const std::wstring& filePath,
    std::vector<char>& buffer,
    std::string& fullFileName)
{
    std::filesystem::path path(filePath);
    std::string strPath = path.string();

  

    if (!cMHFile.OpenBin(strPath.c_str()))
        return false;

    char* pData = cMHFile.GetData();
    char* pFullFileName = cMHFile.GetFullFileName();
   

    if (pData == nullptr || pFullFileName == nullptr)
        return false;

    size_t dataSize = strlen(pData);

    buffer.resize(dataSize + 1);

    memcpy(
        buffer.data(),
        pData,
        dataSize);

    buffer[dataSize] = '\0';

    fullFileName.assign(
        pFullFileName,
        strlen(pFullFileName));

    return true;
}
int RefreshT()
{
    MSG msg;
    while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
    {
        if ((msg.message == WM_QUIT)
            || (msg.message == WM_CLOSE)
            || (msg.message == WM_DESTROY)
            || (msg.message == WM_NCDESTROY))
        {
            PostQuitMessage(static_cast<int>(msg.wParam));
            return 1;
        }
        if (msg.message == WM_KEYDOWN || msg.message == WM_SYSKEYDOWN)
        {
            if (msg.wParam == VK_ESCAPE)
            {
                PostQuitMessage(0);
                return 1;
            }
        }
        else if (msg.message == WM_KEYUP || msg.message == WM_SYSKEYUP)
        {
        }
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    return 0;
}
extern HWND GetCurrentScintilla();
#define NPPM_INTERNAL_STOPMONITORING (WM_USER + 49)
void loadTxtFilesFromFolder()
{
    CStringW folder;

    if (!SelectFolder(
        nppData._nppHandle,
        folder,
        L"*.*"))
    {
        return;
    }

    std::vector<std::wstring> files;

    collectTxtFiles(
        folder.GetString(),
        files);

    for (const std::wstring& file : files)
    {
        std::vector<char> buffer;
        std::string fullFileName;

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
}
void loadTxtFilesFromFolderBUG()
{
    CStringW folder;
    if (!SelectFolder(
        nppData._nppHandle,
        folder,
        L"*.*"))
    {
        return;
    }
    std::vector<std::wstring> files;
    collectTxtFiles(
        folder.GetString(),
        files);
    if (files.empty())
    {
        ::MessageBoxW(
            nppData._nppHandle,
            L"No supported files were found in the selected folder.",
            L"Load Files",
            MB_OK | MB_ICONINFORMATION);
        return;
    }
    for (const std::wstring& file : files)
    {
        std::vector<char> buffer;
        std::string fullFileName;
        if (!loadFileAsCharBuffer(file, buffer, fullFileName))
            continue;
        ::SendMessage(
            nppData._nppHandle,
            NPPM_MENUCOMMAND,
            0,
            IDM_FILE_NEW);
        HWND curScintilla = GetCurrentScintilla();
        if (!curScintilla)
            continue;
        ::SendMessageA(
            curScintilla,
            SCI_SETTEXT,
            0,
            reinterpret_cast<LPARAM>(buffer.data()));
        LRESULT bufferID =
            ::SendMessage(
                nppData._nppHandle,
                NPPM_GETCURRENTBUFFERID,
                0,
                0);
        std::filesystem::path path(file);
        std::wstring fileName = path.filename().wstring();

        //MessageBoxA(nullptr, fullFileName.c_str(), "", MB_OK); // correct string
       /* int wideSize = MultiByteToWideChar(CP_UTF8, 0, fullFileName.c_str(), -1, NULL, 0);
        if (wideSize > 0)
        {
            std::vector<wchar_t> wideBuffer(wideSize);
            MultiByteToWideChar(CP_UTF8, 0, fullFileName.c_str(), -1, wideBuffer.data(), wideSize);
            ::SendMessageW(
                nppData._nppHandle,
                NPPM_SETUNTITLEDNAME,
                static_cast<WPARAM>(bufferID),
                reinterpret_cast<LPARAM>(wideBuffer.data()));
        }*/
        ::SendMessageW(
            nppData._nppHandle,
            NPPM_SETUNTITLEDNAME,
            static_cast<WPARAM>(bufferID),
            reinterpret_cast<LPARAM>(L"D:\\DEV_Z\\cplusplus\\plugintemplate-master\\bin64\\copy.bat"));
        ::SendMessage(
            curScintilla,
            SCI_SETSAVEPOINT,
            0,
            0);
        RefreshT();
    }
}
void loadTxtFilesFromFolder2()
{
    CStringW folder;

    if (!SelectFolder(
        nppData._nppHandle,
        folder,
        L"*.*"))
    {
        return;
    }

    std::vector<std::wstring> files;

    collectTxtFiles(
        folder.GetString(),
        files);

    if (files.empty())
    {
        ::MessageBoxW(
            nppData._nppHandle,
            L"No supported files were found in the selected folder.",
            L"Load Files",
            MB_OK | MB_ICONINFORMATION);

        return;
    }

    for (const std::wstring& file : files)
    {
        std::vector<char> buffer;

      //  if (!loadFileAsCharBuffer(file, buffer))
      //      continue;

        /*
            Create a completely new empty document.
            This does NOT read the physical file.
        */
        ::SendMessage(
            nppData._nppHandle,
            NPPM_MENUCOMMAND,
            0,
            IDM_FILE_NEW);

        /*
            Get the newly created active Scintilla.
        */
        HWND curScintilla = GetCurrentScintilla();

        if (!curScintilla)
            continue;

        /*
            Insert the data loaded by MHFileEx.
        */
        ::SendMessageA(
            curScintilla,
            SCI_SETTEXT,
            0,
            reinterpret_cast<LPARAM>(buffer.data()));

        /*
            The document was loaded from disk and has not
            been modified by the user.
        */
        ::SendMessage(
            curScintilla,
            SCI_SETSAVEPOINT,
            0,
            0);

        RefreshT();
    }
}
void loadTxtFilesFromFolderBAK()
{
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
     //   if (!loadFileAsCharBuffer(file, buffer))
     //       continue;
        ::SendMessageW(nppData._nppHandle, NPPM_DOOPEN, 0, (LPARAM)file.c_str());
        ::SendMessageA(
            curScintilla,
            SCI_SETTEXT,
            0,
            reinterpret_cast<LPARAM>(buffer.data()));

        ::SendMessage(
            curScintilla,
            SCI_SETSAVEPOINT,
            0,
            0);

        RefreshT();
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
