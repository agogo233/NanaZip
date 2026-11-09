/*
 * PROJECT:    NanaZip
 * FILE:       NanaZip.ShellExtension.cpp
 * PURPOSE:    Implementation for NanaZip Shell Extension
 *
 * LICENSE:    The MIT License
 *
 * MAINTAINER: MouriNaruto (Kenji.Mouri@outlook.com)
 */

#include <Windows.h>

#include <shlwapi.h>
#pragma comment(lib, "Shlwapi.lib")

#include <shobjidl_core.h>
#include <shlobj_core.h>
#include <map>
#include <new>
#include <string>
#include <vector>

extern long g_DllRefCount;

#include <winrt/Windows.Foundation.h>

#include "../SevenZip/CPP/Common/Common.h"
#include "../SevenZip/CPP/Windows/DLL.h"
#include "../SevenZip/CPP/Windows/FileDir.h"
#include "../SevenZip/CPP/Windows/FileFind.h"
#include "../SevenZip/CPP/Windows/FileName.h"
#include "../SevenZip/CPP/Windows/ProcessUtils.h"
#include "../SevenZip/CPP/7zip/UI/Common/ArchiveName.h"
#include "../SevenZip/CPP/7zip/UI/Common/CompressCall.h"
#include "../SevenZip/CPP/7zip/UI/Common/ExtractingFilePath.h"
#include "../SevenZip/CPP/7zip/UI/Common/ZipRegistry.h"
#include "../SevenZip/CPP/7zip/UI/FileManager/FormatUtils.h"
#include "../SevenZip/CPP/7zip/UI/FileManager/LangUtils.h"
#include "../SevenZip/CPP/7zip/UI/Explorer/ContextMenuFlags.h"
#include "../SevenZip/CPP/7zip/UI/Explorer/resource.h"

namespace
{
    static const char* const kExtractExcludeExtensions =
        " 3gp"
        " aac ans ape asc asm asp aspx avi awk"
        " bas bat bmp"
        " c cs cls clw cmd cpp csproj css ctl cxx"
        " def dep dlg dsp dsw"
        " eps"
        " f f77 f90 f95 fla flac frm"
        " gif"
        " h hpp hta htm html hxx"
        " ico idl inc ini inl"
        " java jpeg jpg js"
        " la lnk log"
        " mak manifest wmv mov mp3 mp4 mpe mpeg mpg m4a"
        " ofr ogg"
        " pac pas pdf php php3 php4 php5 phptml pl pm png ps py pyo"
        " ra rb rc reg rka rm rtf"
        " sed sh shn shtml sln sql srt swa"
        " tcl tex tiff tta txt"
        " vb vcproj vbs"
        " wav webp wma wv"
        " xml xsd xsl xslt"
        " ";

    static bool FindExt(const char* p, const FString& name)
    {
        int dotPos = name.ReverseFind_Dot();
        if (dotPos < 0 || dotPos == (int)name.Len() - 1)
            return false;

        AString s;

        for (unsigned pos = dotPos + 1;; pos++)
        {
            wchar_t c = name[pos];
            if (c == 0)
                break;
            if (c >= 0x80)
                return false;
            s += (char)MyCharLower_Ascii((char)c);
        }

        for (unsigned i = 0; p[i] != 0;)
        {
            unsigned j;
            for (j = i; p[j] != ' '; j++);
            if (s.Len() == j - i && memcmp(p + i, (const char*)s, s.Len()) == 0)
                return true;
            i = j + 1;
        }

        return false;
    }

    static bool DoNeedExtract(const FString& name)
    {
        return !FindExt(kExtractExcludeExtensions, name);
    }

    static const char* const kArcExts[] =
    {
        "7z"
      , "bz2"
      , "gz"
      , "lz"
      , "liz"
      , "lz4"
      , "lz5"
      , "rar"
      , "zip"
      , "zst"
    };

    static bool IsItArcExt(const UString& ext)
    {
        for (unsigned i = 0; i < ARRAY_SIZE(kArcExts); i++)
            if (ext.IsEqualTo_Ascii_NoCase(kArcExts[i]))
                return true;
        return false;
    }

    UString GetSubFolderNameForExtract(const UString& arcName)
    {
        int dotPos = arcName.ReverseFind_Dot();
        if (dotPos < 0)
            return Get_Correct_FsFile_Name(arcName) + L'~';

        const UString ext = arcName.Ptr(dotPos + 1);
        UString res = arcName.Left(dotPos);
        res.TrimRight();
        dotPos = res.ReverseFind_Dot();
        if (dotPos > 0)
        {
            const UString ext2 = res.Ptr(dotPos + 1);
            if ((ext.IsEqualTo_Ascii_NoCase("001") && IsItArcExt(ext2))
                || (ext.IsEqualTo_Ascii_NoCase("rar") &&
                    (ext2.IsEqualTo_Ascii_NoCase("part001")
                        || ext2.IsEqualTo_Ascii_NoCase("part01")
                        || ext2.IsEqualTo_Ascii_NoCase("part1"))))
                res.DeleteFrom(dotPos);
            res.TrimRight();
        }
        return Get_Correct_FsFile_Name(res);
    }

    static void ReduceString(UString& s)
    {
        const unsigned kMaxSize = 60;
        if (s.Len() <= kMaxSize)
            return;
        s.Delete(kMaxSize / 2, s.Len() - kMaxSize);
        s.Insert(kMaxSize / 2, L" ... ");
    }

    // Backport GetQuotedReducedString from 7-Zip ZS since GetQuotedString
    // now does string escaping as well.
    static UString GetQuotedReducedString(const UString& s)
    {
        UString s2 = s;
        ReduceString(s2);
        s2.Replace(L"&", L"&&");
        s2.InsertAtFront(L'"');
        s2 += L'"'; // quote without GetQuotedString (because it escapes now)
        return s2;
    }

    static void MyFormatNew_ReducedName(UString& s, const UString& name)
    {
        s = MyFormatNew(s, GetQuotedReducedString(name));
    }

    static UString GetNanaZipPath()
    {
        return fs2us(NWindows::NDLL::GetModuleDirPrefix()) + L"NanaZip.Modern.FileManager.exe";
    }
}

namespace NanaZip::ShellExtension
{
    namespace CommandID
    {
        enum
        {
            None,

            Open,
            Test,

            Extract,
            ExtractHere,
            ExtractHereSmart,
            ExtractTo,

            Compress,
            CompressTo7z,
            CompressToZip,

            CompressEmail,
            CompressTo7zEmail,
            CompressToZipEmail,

            HashCRC32,
            HashCRC64,
            HashSHA1,
            HashSHA256,
            HashAll,

            Maximum
        };
    }

    using SubCommandList = std::vector<winrt::com_ptr<IExplorerCommand>>;
    using SubCommandListIterator = SubCommandList::const_iterator;

    struct ExplorerCommandBase : public winrt::implements<
        ExplorerCommandBase,
        IExplorerCommand>
    {
    private:

        std::wstring m_Title;

        DWORD m_CommandID;
        bool m_IsSeparator;
        CBoolPair m_ElimDup;
        UInt32 m_WriteZone;

    public:

        ExplorerCommandBase(
            std::wstring const& Title = std::wstring(),
            DWORD CommandID = CommandID::None,
            CBoolPair const& ElimDup = CBoolPair(),
            UInt32 const& WriteZone = static_cast<UInt32>(-1)) :
            m_Title(Title),
            m_CommandID(CommandID),
            m_ElimDup(ElimDup),
            m_WriteZone(WriteZone)
        {
            this->m_IsSeparator = (this->m_CommandID == CommandID::None);
        }

#pragma region IExplorerCommand

        HRESULT STDMETHODCALLTYPE GetTitle(
            _In_opt_ IShellItemArray* psiItemArray,
            _Outptr_ LPWSTR* ppszName)
        {
            UNREFERENCED_PARAMETER(psiItemArray);

            if (this->m_IsSeparator)
            {
                *ppszName = nullptr;
                return S_FALSE;
            }

            return ::SHStrDupW(this->m_Title.c_str(), ppszName);
        }

        HRESULT STDMETHODCALLTYPE GetIcon(
            _In_opt_ IShellItemArray* psiItemArray,
            _Outptr_ LPWSTR* ppszIcon)
        {
            UNREFERENCED_PARAMETER(psiItemArray);

            *ppszIcon = nullptr;
            return E_NOTIMPL;
        }

        HRESULT STDMETHODCALLTYPE GetToolTip(
            _In_opt_ IShellItemArray* psiItemArray,
            _Outptr_ LPWSTR* ppszInfotip)
        {
            UNREFERENCED_PARAMETER(psiItemArray);
            *ppszInfotip = nullptr;
            return E_NOTIMPL;
        }

        HRESULT STDMETHODCALLTYPE GetCanonicalName(
            _Out_ GUID* pguidCommandName)
        {
            *pguidCommandName = GUID_NULL;
            return E_NOTIMPL;
        }

        HRESULT STDMETHODCALLTYPE GetState(
            _In_opt_ IShellItemArray* psiItemArray,
            _In_ BOOL fOkToBeSlow,
            _Out_ EXPCMDSTATE* pCmdState)
        {
            UNREFERENCED_PARAMETER(psiItemArray);
            UNREFERENCED_PARAMETER(fOkToBeSlow);
            *pCmdState = ECS_ENABLED;
            return S_OK;
        }

        HRESULT STDMETHODCALLTYPE Invoke(
            _In_opt_ IShellItemArray* psiItemArray,
            _In_opt_ IBindCtx* pbc)
        {
            UNREFERENCED_PARAMETER(pbc);

            if (this->m_IsSeparator)
            {
                return E_NOTIMPL;
            }

            std::vector<std::wstring> FilePaths;
            if (psiItemArray)
            {
                DWORD Count = 0;
                if (SUCCEEDED(psiItemArray->GetCount(&Count)))
                {
                    for (DWORD i = 0; i < Count; ++i)
                    {
                        winrt::com_ptr<IShellItem> Item;
                        if (SUCCEEDED(psiItemArray->GetItemAt(
                            i,
                            Item.put())))
                        {
                            LPWSTR DisplayName = nullptr;
                            if (SUCCEEDED(Item->GetDisplayName(
                                SIGDN_FILESYSPATH,
                                &DisplayName)))
                            {
                                FilePaths.push_back(std::wstring(DisplayName));
                                ::CoTaskMemFree(DisplayName);
                            }
                        }
                    }
                }
            }

            bool NeedExtract = false;
            if (FilePaths.size() > 0)
            {
                for (std::wstring const FilePath : FilePaths)
                {
                    DWORD FileAttributes = ::GetFileAttributesW(
                        FilePath.c_str());
                    if (FileAttributes & FILE_ATTRIBUTE_DIRECTORY)
                    {
                        continue;
                    }

                    if (DoNeedExtract(::PathFindFileNameW(FilePath.c_str())))
                    {
                        NeedExtract = true;
                        break;
                    }
                }
            }

            std::wstring SpecFolder = L"*";
            if (NeedExtract)
            {
                if (FilePaths.size() == 1)
                {
                    SpecFolder = GetSubFolderNameForExtract(
                        ::PathFindFileNameW(FilePaths[0].c_str()));
                }
                SpecFolder += L'\\';
            }

            UStringVector FileNames;
            for (std::wstring const FilePath : FilePaths)
            {
                FileNames.Add(FilePath.c_str());
            }

            FString FolderPrefix;

            std::wstring ArchiveName;
            if (FilePaths.size() > 0)
            {
                NWindows::NFile::NFind::CFileInfo FileInfo0;

                const UString& FileName = FileNames.Front();

                if (NWindows::NFile::NName::IsDevicePath(us2fs(FileName)))
                {
                    // CFileInfo::Find can be slow for device files. So we
                    // don't call it.
                    // we need only name here.
                    // change it 4 - must be constant
                    FileInfo0.Name = us2fs(FileName.Ptr(
                        NWindows::NFile::NName::kDevicePathPrefixSize));
                    FolderPrefix = "C:\\";
                }
                else
                {
                    if (!FileInfo0.Find(us2fs(FileName)))
                    {
                        return ::HRESULT_FROM_WIN32(::GetLastError());
                    }
                    NWindows::NFile::NDir::GetOnlyDirPrefix(
                        us2fs(FileName),
                        FolderPrefix);
                }

                const UString Name = CreateArchiveName(
                    FileNames,
                    FileNames.Size() == 1 ? &FileInfo0 : nullptr);
                ArchiveName = std::wstring(Name.Ptr(), Name.Len());

            }

            std::wstring BaseFolder = std::wstring(
                FolderPrefix.Ptr(),
                FolderPrefix.Len());

            std::wstring ArchiveName7z = ArchiveName + L".7z";
            std::wstring ArchiveNameZip = ArchiveName + L".zip";

            switch (this->m_CommandID)
            {
            case CommandID::Open:
            {
                if (FilePaths.size() != 1)
                {
                    break;
                }

                DWORD FileAttributes = ::GetFileAttributesW(
                    FilePaths[0].c_str());
                if (FileAttributes & FILE_ATTRIBUTE_DIRECTORY)
                {
                    break;
                }

                if (!DoNeedExtract(FilePaths[0].c_str()))
                {
                    break;
                }

                UString params;
                params = GetQuotedString(FilePaths[0].c_str());
                NWindows::MyCreateProcess(::GetNanaZipPath(), params);

                break;
            }
            case CommandID::Test:
            {
                if (!NeedExtract)
                {
                    break;
                }

                TestArchives(FileNames);
                break;
            }
            case CommandID::Extract:
            case CommandID::ExtractHere:
            case CommandID::ExtractHereSmart:
            case CommandID::ExtractTo:
            {
                if (!NeedExtract)
                {
                    break;
                }

                std::wstring Folder = BaseFolder;
                if (this->m_CommandID != CommandID::ExtractHere &&
                    this->m_CommandID != CommandID::ExtractHereSmart)
                {
                    Folder += SpecFolder;
                }

                ExtractArchives(
                    FileNames,
                    Folder.c_str(),
                    (this->m_CommandID == CommandID::Extract),
                    ((this->m_CommandID == CommandID::ExtractTo)
                    && this->m_ElimDup.Val),
                    this->m_WriteZone,
                    (this->m_CommandID == CommandID::ExtractHereSmart));

                break;
            }
            case CommandID::Compress:
            case CommandID::CompressTo7z:
            case CommandID::CompressToZip:
            case CommandID::CompressEmail:
            case CommandID::CompressTo7zEmail:
            case CommandID::CompressToZipEmail:
            {
                bool Email =(
                    (this->m_CommandID == CommandID::CompressEmail) ||
                    (this->m_CommandID == CommandID::CompressTo7zEmail) ||
                    (this->m_CommandID == CommandID::CompressToZipEmail));
                bool ShowDialog = (
                    (this->m_CommandID == CommandID::Compress) ||
                    (this->m_CommandID == CommandID::CompressEmail));
                bool AddExtension = (
                    (this->m_CommandID == CommandID::Compress) ||
                    (this->m_CommandID == CommandID::CompressEmail));
                bool Is7z = (
                    (this->m_CommandID == CommandID::CompressTo7z) ||
                    (this->m_CommandID == CommandID::CompressTo7zEmail));

                std::wstring Name = (
                    AddExtension
                    ? ArchiveName
                    : (Is7z ? ArchiveName7z : ArchiveNameZip));

                CompressFiles(
                    BaseFolder.c_str(),
                    Name.c_str(),
                    Is7z ? L"7z" : L"zip",
                    AddExtension,
                    FileNames,
                    Email,
                    ShowDialog,
                    false);

                break;
            }
            case CommandID::HashCRC32:
            case CommandID::HashCRC64:
            case CommandID::HashSHA1:
            case CommandID::HashSHA256:
            case CommandID::HashAll:
            {
                std::wstring MethodName;
                switch (this->m_CommandID)
                {
                case CommandID::HashCRC32:
                    MethodName = L"CRC32";
                    break;
                case CommandID::HashCRC64:
                    MethodName = L"CRC64";
                    break;
                case CommandID::HashSHA1:
                    MethodName = L"SHA1";
                    break;
                case CommandID::HashSHA256:
                    MethodName = L"SHA256";
                    break;
                case CommandID::HashAll:
                    MethodName = L"*";
                    break;
                default:
                    break;
                }

                CalcChecksum(FileNames, MethodName.c_str(), L"", L"");
                break;
            }
            default:
                break;
            }

            return S_OK;
        }

        HRESULT STDMETHODCALLTYPE GetFlags(
            _Out_ EXPCMDFLAGS* pFlags)
        {
            *pFlags =
                this->m_IsSeparator
                ? ECF_ISSEPARATOR
                : ECF_DEFAULT;
            return S_OK;
        }

        HRESULT STDMETHODCALLTYPE EnumSubCommands(
            _Outptr_ IEnumExplorerCommand** ppEnum)
        {
            *ppEnum = nullptr;
            return E_NOTIMPL;
        }

#pragma endregion
    };


    struct ExplorerCommandRoot : public winrt::implements<
        ExplorerCommandRoot,
        IExplorerCommand,
        IEnumExplorerCommand>
    {
    private:

        DWORD m_ContextMenuFlags;
        CBoolPair m_ContextMenuElimDup;

        bool m_Initialized = false;
        SubCommandList m_SubCommands;
        SubCommandListIterator m_CurrentSubCommand;
        bool m_SelectionValid = false;

        void Initialize(
            _In_opt_ IShellItemArray* psiItemArray)
        {
            if (m_Initialized)
            {
                return;
            }

            m_Initialized = true;

            std::vector<std::wstring> FilePaths;
            if (psiItemArray)
            {
                DWORD Count = 0;
                if (SUCCEEDED(psiItemArray->GetCount(&Count)))
                {
                    for (DWORD i = 0; i < Count; ++i)
                    {
                        winrt::com_ptr<IShellItem> Item;
                        if (SUCCEEDED(psiItemArray->GetItemAt(
                            i,
                            Item.put())))
                        {
                            LPWSTR DisplayName = nullptr;
                            if (SUCCEEDED(Item->GetDisplayName(
                                SIGDN_FILESYSPATH,
                                &DisplayName)))
                            {
                                FilePaths.push_back(std::wstring(DisplayName));
                                ::CoTaskMemFree(DisplayName);
                            }
                        }
                    }
                }
            }

            if (FilePaths.empty())
            {
                this->m_SelectionValid = false;
                return;
            }
            this->m_SelectionValid = true;

            UStringVector FileNames;
            for (std::wstring const FilePath : FilePaths)
            {
                FileNames.Add(FilePath.c_str());
            }

            bool NeedExtract = false;
            for (std::wstring const FilePath : FilePaths)
            {
                DWORD FileAttributes = ::GetFileAttributesW(
                    FilePath.c_str());
                if (FileAttributes & FILE_ATTRIBUTE_DIRECTORY)
                {
                    continue;
                }

                if (DoNeedExtract(::PathFindFileNameW(FilePath.c_str())))
                {
                    NeedExtract = true;
                    break;
                }
            }

            std::wstring SpecFolder = L"*";
            if (NeedExtract)
            {
                if (FilePaths.size() == 1)
                {
                    SpecFolder = GetSubFolderNameForExtract(
                        ::PathFindFileNameW(FilePaths[0].c_str()));
                }
                SpecFolder += L'\\';
            }

            FString FolderPrefix;

            std::wstring ArchiveName;
            {
                NWindows::NFile::NFind::CFileInfo FileInfo0;

                const UString& FileName = FileNames.Front();

                if (NWindows::NFile::NName::IsDevicePath(us2fs(FileName)))
                {
                    // CFileInfo::Find can be slow for device files. So we
                    // don't call it.
                    // we need only name here.
                    // change it 4 - must be constant
                    FileInfo0.Name = us2fs(FileName.Ptr(
                        NWindows::NFile::NName::kDevicePathPrefixSize));
                    FolderPrefix = "C:\\";
                }
                else
                {
                    if (!FileInfo0.Find(us2fs(FileName)))
                    {
                        return;
                    }
                    NWindows::NFile::NDir::GetOnlyDirPrefix(
                        us2fs(FileName),
                        FolderPrefix);
                }

                const UString Name = CreateArchiveName(
                    FileNames,
                    FileNames.Size() == 1 ? &FileInfo0 : nullptr);
                ArchiveName = std::wstring(Name.Ptr(), Name.Len());

            }

            std::wstring BaseFolder = std::wstring(
                FolderPrefix.Ptr(),
                FolderPrefix.Len());

            std::wstring ArchiveName7z = ArchiveName + L".7z";
            std::wstring ArchiveNameZip = ArchiveName + L".zip";

            using NanaZip::ShellExtension::ExplorerCommandBase;

            CContextMenuInfo ContextMenuInfo;
            ContextMenuInfo.Load();
            DWORD ContextMenuFlags = ContextMenuInfo.Flags;
            CBoolPair ContextMenuElimDup = ContextMenuInfo.ElimDup;
            UInt32 ContextMenuWriteZone = ContextMenuInfo.WriteZone;

            LoadLangOneTime();

            if (ContextMenuFlags & NContextMenuFlags::kOpen)
            {
                DWORD FileAttributes = ::GetFileAttributesW(
                    FilePaths[0].c_str());
                if ((FilePaths.size() == 1) &&
                    !(FileAttributes & FILE_ATTRIBUTE_DIRECTORY) &&
                    DoNeedExtract(FilePaths[0].c_str()))
                {
                    UString TranslatedString;
                    LangString(IDS_CONTEXT_OPEN, TranslatedString);
                    this->m_SubCommands.push_back(
                        winrt::make<ExplorerCommandBase>(
                            std::wstring(
                                TranslatedString.Ptr(),
                                TranslatedString.Len()),
                            CommandID::Open));
                }
            }

            if (NeedExtract)
            {
                if (ContextMenuFlags & NContextMenuFlags::kTest)
                {
                    UString TranslatedString;
                    LangString(IDS_CONTEXT_TEST, TranslatedString);
                    this->m_SubCommands.push_back(
                        winrt::make<ExplorerCommandBase>(
                            std::wstring(
                                TranslatedString.Ptr(),
                                TranslatedString.Len()),
                            CommandID::Test));
                }

                if (ContextMenuFlags & NContextMenuFlags::kExtract)
                {
                    UString TranslatedString;
                    LangString(IDS_CONTEXT_EXTRACT, TranslatedString);
                    this->m_SubCommands.push_back(
                        winrt::make<ExplorerCommandBase>(
                            std::wstring(
                                TranslatedString.Ptr(),
                                TranslatedString.Len()),
                            CommandID::Extract,
                            ContextMenuElimDup,
                            ContextMenuWriteZone));
                }

                if (ContextMenuFlags & NContextMenuFlags::kExtractHere)
                {
                    UString TranslatedString;
                    LangString(IDS_CONTEXT_EXTRACT_HERE, TranslatedString);
                    this->m_SubCommands.push_back(
                        winrt::make<ExplorerCommandBase>(
                            std::wstring(
                                TranslatedString.Ptr(),
                                TranslatedString.Len()),
                            CommandID::ExtractHere,
                            ContextMenuElimDup,
                            ContextMenuWriteZone));
                }

                if (ContextMenuFlags & NContextMenuFlags::kExtractHereSmart)
                {
                    UString TranslatedString;
                    LangString(IDS_CONTEXT_EXTRACT_HERE_SMART, TranslatedString);
                    this->m_SubCommands.push_back(
                        winrt::make<ExplorerCommandBase>(
                            std::wstring(
                                TranslatedString.Ptr(),
                                TranslatedString.Len()),
                            CommandID::ExtractHereSmart,
                            ContextMenuElimDup,
                            ContextMenuWriteZone));
                }

                if (ContextMenuFlags & NContextMenuFlags::kExtractTo)
                {
                    UString TranslatedString;
                    LangString(IDS_CONTEXT_EXTRACT_TO, TranslatedString);
                    MyFormatNew_ReducedName(TranslatedString, SpecFolder.c_str());
                    this->m_SubCommands.push_back(
                        winrt::make<ExplorerCommandBase>(
                            std::wstring(
                                TranslatedString.Ptr(),
                                TranslatedString.Len()),
                            CommandID::ExtractTo,
                            ContextMenuElimDup,
                            ContextMenuWriteZone));
                }
            }

            if (ContextMenuFlags & NContextMenuFlags::kCompress)
            {
                UString TranslatedString;
                LangString(IDS_CONTEXT_COMPRESS, TranslatedString);
                this->m_SubCommands.push_back(
                    winrt::make<ExplorerCommandBase>(
                        std::wstring(
                            TranslatedString.Ptr(),
                            TranslatedString.Len()),
                        CommandID::Compress));
            }

            if (ContextMenuFlags & NContextMenuFlags::kCompressTo7z)
            {
                UString TranslatedString;
                LangString(IDS_CONTEXT_COMPRESS_TO, TranslatedString);
                MyFormatNew_ReducedName(TranslatedString, ArchiveName7z.c_str());
                this->m_SubCommands.push_back(
                    winrt::make<ExplorerCommandBase>(
                        std::wstring(
                            TranslatedString.Ptr(),
                            TranslatedString.Len()),
                        CommandID::CompressTo7z));
            }

            if (ContextMenuFlags & NContextMenuFlags::kCompressToZip)
            {
                UString TranslatedString;
                LangString(IDS_CONTEXT_COMPRESS_TO, TranslatedString);
                MyFormatNew_ReducedName(TranslatedString, ArchiveNameZip.c_str());
                this->m_SubCommands.push_back(
                    winrt::make<ExplorerCommandBase>(
                        std::wstring(
                            TranslatedString.Ptr(),
                            TranslatedString.Len()),
                        CommandID::CompressToZip));
            }

            if (ContextMenuFlags & NContextMenuFlags::kCompressEmail)
            {
                UString TranslatedString;
                LangString(IDS_CONTEXT_COMPRESS_EMAIL, TranslatedString);
                this->m_SubCommands.push_back(
                    winrt::make<ExplorerCommandBase>(
                        std::wstring(
                            TranslatedString.Ptr(),
                            TranslatedString.Len()),
                        CommandID::CompressEmail));
            }

            if (ContextMenuFlags & NContextMenuFlags::kCompressTo7zEmail)
            {
                UString TranslatedString;
                LangString(IDS_CONTEXT_COMPRESS_TO_EMAIL, TranslatedString);
                MyFormatNew_ReducedName(TranslatedString, ArchiveName7z.c_str());
                this->m_SubCommands.push_back(
                    winrt::make<ExplorerCommandBase>(
                        std::wstring(
                            TranslatedString.Ptr(),
                            TranslatedString.Len()),
                        CommandID::CompressTo7zEmail));
            }

            if (ContextMenuFlags & NContextMenuFlags::kCompressToZipEmail)
            {
                UString TranslatedString;
                LangString(IDS_CONTEXT_COMPRESS_TO_EMAIL, TranslatedString);
                MyFormatNew_ReducedName(TranslatedString, ArchiveNameZip.c_str());
                this->m_SubCommands.push_back(
                    winrt::make<ExplorerCommandBase>(
                        std::wstring(
                            TranslatedString.Ptr(),
                            TranslatedString.Len()),
                        CommandID::CompressToZipEmail));
            }

            if (ContextMenuFlags & NContextMenuFlags::kCRC)
            {
                if (!this->m_SubCommands.empty())
                {
                    this->m_SubCommands.push_back(
                        winrt::make<ExplorerCommandBase>());
                }

                this->m_SubCommands.push_back(
                    winrt::make<ExplorerCommandBase>(
                        L"CRC-32",
                        CommandID::HashCRC32));

                this->m_SubCommands.push_back(
                    winrt::make<ExplorerCommandBase>(
                        L"CRC-64",
                        CommandID::HashCRC64));

                this->m_SubCommands.push_back(
                    winrt::make<ExplorerCommandBase>(
                        L"SHA-1",
                        CommandID::HashSHA1));

                this->m_SubCommands.push_back(
                    winrt::make<ExplorerCommandBase>(
                        L"SHA-256",
                        CommandID::HashSHA256));

                this->m_SubCommands.push_back(
                    winrt::make<ExplorerCommandBase>(
                        L"*",
                        CommandID::HashAll));
            }
        }

        void PopulateDefaultCommands()
        {
            if (!this->m_SubCommands.empty())
            {
                return;
            }

            LoadLangOneTime();

            if (this->m_ContextMenuFlags & NContextMenuFlags::kCompress)
            {
                UString TranslatedString;
                LangString(IDS_CONTEXT_COMPRESS, TranslatedString);
                this->m_SubCommands.push_back(
                    winrt::make<ExplorerCommandBase>(
                        std::wstring(
                            TranslatedString.Ptr(),
                            TranslatedString.Len()),
                        CommandID::Compress));
            }

            if (this->m_ContextMenuFlags & NContextMenuFlags::kCompressEmail)
            {
                UString TranslatedString;
                LangString(IDS_CONTEXT_COMPRESS_EMAIL, TranslatedString);
                this->m_SubCommands.push_back(
                    winrt::make<ExplorerCommandBase>(
                        std::wstring(
                            TranslatedString.Ptr(),
                            TranslatedString.Len()),
                        CommandID::CompressEmail));
            }

            if (this->m_ContextMenuFlags & NContextMenuFlags::kCRC)
            {
                if (!this->m_SubCommands.empty())
                {
                    this->m_SubCommands.push_back(
                        winrt::make<ExplorerCommandBase>());
                }

                this->m_SubCommands.push_back(
                    winrt::make<ExplorerCommandBase>(
                        L"CRC-32",
                        CommandID::HashCRC32));
                this->m_SubCommands.push_back(
                    winrt::make<ExplorerCommandBase>(
                        L"CRC-64",
                        CommandID::HashCRC64));
                this->m_SubCommands.push_back(
                    winrt::make<ExplorerCommandBase>(
                        L"SHA-1",
                        CommandID::HashSHA1));
                this->m_SubCommands.push_back(
                    winrt::make<ExplorerCommandBase>(
                        L"SHA-256",
                        CommandID::HashSHA256));
                this->m_SubCommands.push_back(
                    winrt::make<ExplorerCommandBase>(
                        L"*",
                        CommandID::HashAll));
            }

            if (this->m_SubCommands.empty())
            {
                UString TranslatedString;
                LangString(IDS_CONTEXT_COMPRESS, TranslatedString);
                this->m_SubCommands.push_back(
                    winrt::make<ExplorerCommandBase>(
                        std::wstring(
                            TranslatedString.Ptr(),
                            TranslatedString.Len()),
                        CommandID::Compress));
            }
        }

    public:

        ExplorerCommandRoot()
        {
            CContextMenuInfo ContextMenuInfo;
            ContextMenuInfo.Load();
            this->m_ContextMenuFlags = ContextMenuInfo.Flags;
            this->m_ContextMenuElimDup = ContextMenuInfo.ElimDup;
        }

#pragma region IExplorerCommand

        HRESULT STDMETHODCALLTYPE GetTitle(
            _In_opt_ IShellItemArray* psiItemArray,
            _Outptr_ LPWSTR* ppszName)
        {
            this->Initialize(psiItemArray);

            if (this->m_SubCommands.empty() && !this->m_SelectionValid)
            {
                this->PopulateDefaultCommands();
            }

            if (this->m_SubCommands.empty())
            {
                *ppszName = nullptr;
                return E_NOTIMPL;
            }

            return ::SHStrDupW(L"NanaZip Preview", ppszName);
        }

        HRESULT STDMETHODCALLTYPE GetIcon(
            _In_opt_ IShellItemArray* psiItemArray,
            _Outptr_ LPWSTR* ppszIcon)
        {
            UNREFERENCED_PARAMETER(psiItemArray);
            UString Path = ::GetNanaZipPath();
            std::wstring Icon = std::wstring(Path.Ptr(), Path.Len());
            Icon += L",-1";
            return ::SHStrDupW(Icon.c_str(), ppszIcon);
        }

        HRESULT STDMETHODCALLTYPE GetToolTip(
            _In_opt_ IShellItemArray* psiItemArray,
            _Outptr_ LPWSTR* ppszInfotip)
        {
            UNREFERENCED_PARAMETER(psiItemArray);
            *ppszInfotip = nullptr;
            return E_NOTIMPL;
        }

        HRESULT STDMETHODCALLTYPE GetCanonicalName(
            _Out_ GUID* pguidCommandName)
        {
            *pguidCommandName = GUID_NULL;
            return E_NOTIMPL;
        }

        HRESULT STDMETHODCALLTYPE GetState(
            _In_opt_ IShellItemArray* psiItemArray,
            _In_ BOOL fOkToBeSlow,
            _Out_ EXPCMDSTATE* pCmdState)
        {
            UNREFERENCED_PARAMETER(psiItemArray);
            UNREFERENCED_PARAMETER(fOkToBeSlow);
            *pCmdState = ECS_ENABLED;
            return S_OK;
        }

        HRESULT STDMETHODCALLTYPE Invoke(
            _In_opt_ IShellItemArray* psiItemArray,
            _In_opt_ IBindCtx* pbc)
        {
            UNREFERENCED_PARAMETER(psiItemArray);
            UNREFERENCED_PARAMETER(pbc);
            return E_NOTIMPL;
        }

        HRESULT STDMETHODCALLTYPE GetFlags(
            _Out_ EXPCMDFLAGS* pFlags)
        {
            *pFlags = ECF_HASSUBCOMMANDS;
            return S_OK;
        }

        HRESULT STDMETHODCALLTYPE EnumSubCommands(
            _Outptr_ IEnumExplorerCommand** ppEnum)
        {
            if (this->m_SubCommands.empty())
            {
                *ppEnum = nullptr;
                return E_NOTIMPL;
            }
            else
            {
                this->m_CurrentSubCommand = this->m_SubCommands.cbegin();
                return this->QueryInterface(IID_PPV_ARGS(ppEnum));
            }
        }

#pragma endregion

#pragma region IEnumExplorerCommand

        HRESULT STDMETHODCALLTYPE Next(
            _In_ ULONG celt,
            _Out_ IExplorerCommand** pUICommand,
            _Out_opt_ ULONG* pceltFetched)
        {
            ULONG Fetched = 0;

            for (
                ULONG i = 0;
                (i < celt) &&
                (this->m_CurrentSubCommand != this->m_SubCommands.cend());
                ++i)
            {
                this->m_CurrentSubCommand->copy_to(&pUICommand[i]);
                ++Fetched;
                ++this->m_CurrentSubCommand;
            }

            if (pceltFetched)
            {
                *pceltFetched = Fetched;
            }

            return (Fetched == celt) ? S_OK : S_FALSE;
        }

        HRESULT STDMETHODCALLTYPE Skip(
            _In_ ULONG celt)
        {
            UNREFERENCED_PARAMETER(celt);
            return E_NOTIMPL;
        }

        HRESULT STDMETHODCALLTYPE Reset()
        {
            this->m_CurrentSubCommand = this->m_SubCommands.cbegin();
            return S_OK;
        }

        HRESULT STDMETHODCALLTYPE Clone(
            _Out_ IEnumExplorerCommand** ppenum)
        {
            *ppenum = nullptr;
            return E_NOTIMPL;
        }

#pragma endregion
    };

    class NanaZipContextMenu final : public IContextMenu, public IShellExtInit
    {
    public:

        NanaZipContextMenu() : m_ref(1)
        {
            ::InterlockedIncrement(&::g_DllRefCount);
        }

        NanaZipContextMenu(NanaZipContextMenu const&) = delete;
        NanaZipContextMenu& operator=(NanaZipContextMenu const&) = delete;

        HRESULT STDMETHODCALLTYPE QueryInterface(
            _In_ REFIID riid,
            _COM_Outptr_ void** ppvObject) override
        {
            if (!ppvObject)
            {
                return E_POINTER;
            }

            *ppvObject = nullptr;

            if (riid == IID_IUnknown || riid == IID_IContextMenu)
            {
                *ppvObject = static_cast<IContextMenu*>(this);
            }
            else if (riid == IID_IShellExtInit)
            {
                *ppvObject = static_cast<IShellExtInit*>(this);
            }
            else
            {
                return E_NOINTERFACE;
            }

            this->AddRef();
            return S_OK;
        }

        ULONG STDMETHODCALLTYPE AddRef() override
        {
            return static_cast<ULONG>(::InterlockedIncrement(&this->m_ref));
        }

        ULONG STDMETHODCALLTYPE Release() override
        {
            ULONG RefCount = static_cast<ULONG>(
                ::InterlockedDecrement(&this->m_ref));
            if (!RefCount)
            {
                delete this;
            }

            return RefCount;
        }

    private:

        ~NanaZipContextMenu()
        {
            ::InterlockedDecrement(&::g_DllRefCount);
        }

        LONG m_ref;
        winrt::com_ptr<IShellItemArray> m_shellItemArray;
        std::map<UINT, DWORD> m_commands;

        static bool GetNeedExtract(
            IShellItemArray* ShellItemArray,
            std::vector<std::wstring>& FilePaths)
        {
            if (!ShellItemArray)
            {
                return false;
            }

            DWORD Count = 0;
            if (FAILED(ShellItemArray->GetCount(&Count)))
            {
                return false;
            }

            for (DWORD i = 0; i < Count; ++i)
            {
                winrt::com_ptr<IShellItem> Item;
                if (SUCCEEDED(ShellItemArray->GetItemAt(i, Item.put())))
                {
                    LPWSTR DisplayName = nullptr;
                    if (SUCCEEDED(Item->GetDisplayName(
                        SIGDN_FILESYSPATH,
                        &DisplayName)))
                    {
                        FilePaths.push_back(std::wstring(DisplayName));
                        ::CoTaskMemFree(DisplayName);
                    }
                }
            }

            for (std::wstring const& FilePath : FilePaths)
            {
                DWORD FileAttributes = ::GetFileAttributesW(
                    FilePath.c_str());
                if (FileAttributes & FILE_ATTRIBUTE_DIRECTORY)
                {
                    continue;
                }

                if (DoNeedExtract(::PathFindFileNameW(FilePath.c_str())))
                {
                    return true;
                }
            }

            return false;
        }

        static UString GetCommandTitle(DWORD CommandID)
        {
            UString Text;
            switch (CommandID)
            {
            case CommandID::Open:
                LangString(IDS_CONTEXT_OPEN, Text);
                break;
            case CommandID::ExtractHere:
                LangString(IDS_CONTEXT_EXTRACT_HERE, Text);
                break;
            case CommandID::ExtractTo:
                LangString(IDS_CONTEXT_EXTRACT_TO, Text);
                break;
            case CommandID::CompressTo7z:
                LangString(IDS_CONTEXT_COMPRESS_TO, Text);
                Text += L" (.7z)";
                break;
            case CommandID::CompressToZip:
                LangString(IDS_CONTEXT_COMPRESS_TO, Text);
                Text += L" (.zip)";
                break;
            case CommandID::Test:
                LangString(IDS_CONTEXT_TEST, Text);
                break;
            case CommandID::HashCRC32:
                Text = L"CRC-32";
                break;
            case CommandID::HashCRC64:
                Text = L"CRC-64";
                break;
            case CommandID::HashSHA1:
                Text = L"SHA-1";
                break;
            case CommandID::HashSHA256:
                Text = L"SHA-256";
                break;
            case CommandID::HashAll:
                Text = L"*";
                break;
            default:
                break;
            }
            return Text;
        }

    public:

        HRESULT STDMETHODCALLTYPE QueryContextMenu(
            _In_ HMENU hMenu,
            _In_ UINT indexMenu,
            _In_ UINT idCmdFirst,
            _In_ UINT idCmdLast,
            _In_ UINT uFlags) override
        {
            UNREFERENCED_PARAMETER(uFlags);

            if (!hMenu)
            {
                return MAKE_HRESULT(SEVERITY_SUCCESS, 0, 0);
            }

            if (idCmdFirst > idCmdLast)
            {
                return E_INVALIDARG;
            }

            if (!this->m_shellItemArray)
            {
                return MAKE_HRESULT(SEVERITY_SUCCESS, 0, 0);
            }

            LoadLangOneTime();

            CContextMenuInfo ContextMenuInfo;
            ContextMenuInfo.Load();
            DWORD ContextMenuFlags = ContextMenuInfo.Flags;

            std::vector<std::wstring> FilePaths;
            bool NeedExtract = GetNeedExtract(
                this->m_shellItemArray.get(),
                FilePaths);
            if (FilePaths.empty())
            {
                return MAKE_HRESULT(SEVERITY_SUCCESS, 0, 0);
            }

            this->m_commands.clear();

            HMENU SubMenu = ::CreatePopupMenu();
            if (!SubMenu)
            {
                return E_OUTOFMEMORY;
            }

            UINT CurrentID = idCmdFirst;
            bool HasItems = false;

            auto AddCommand =
                [&](DWORD CommandID, UString const& Text) -> void
            {
                if (CurrentID >= idCmdLast)
                {
                    return;
                }
                if (Text.Len() == 0)
                {
                    return;
                }
                ::AppendMenuW(
                    SubMenu,
                    MF_STRING,
                    CurrentID,
                    Text.Ptr());
                this->m_commands[CurrentID - idCmdFirst] = CommandID;
                ++CurrentID;
                HasItems = true;
            };

            if ((ContextMenuFlags & NContextMenuFlags::kOpen) &&
                NeedExtract &&
                FilePaths.size() == 1)
            {
                AddCommand(
                    CommandID::Open,
                    GetCommandTitle(CommandID::Open));
            }

            if ((ContextMenuFlags & NContextMenuFlags::kExtractHere) &&
                NeedExtract)
            {
                AddCommand(
                    CommandID::ExtractHere,
                    GetCommandTitle(CommandID::ExtractHere));
            }

            if ((ContextMenuFlags & NContextMenuFlags::kExtractTo) &&
                NeedExtract)
            {
                AddCommand(
                    CommandID::ExtractTo,
                    GetCommandTitle(CommandID::ExtractTo));
            }

            if (ContextMenuFlags & NContextMenuFlags::kCompressTo7z)
            {
                AddCommand(
                    CommandID::CompressTo7z,
                    GetCommandTitle(CommandID::CompressTo7z));
            }

            if (ContextMenuFlags & NContextMenuFlags::kCompressToZip)
            {
                AddCommand(
                    CommandID::CompressToZip,
                    GetCommandTitle(CommandID::CompressToZip));
            }

            if ((ContextMenuFlags & NContextMenuFlags::kTest) &&
                NeedExtract)
            {
                AddCommand(
                    CommandID::Test,
                    GetCommandTitle(CommandID::Test));
            }

            if (ContextMenuFlags & NContextMenuFlags::kCRC)
            {
                AddCommand(CommandID::HashCRC32, L"CRC-32");
                AddCommand(CommandID::HashCRC64, L"CRC-64");
                AddCommand(CommandID::HashSHA1, L"SHA-1");
                AddCommand(CommandID::HashSHA256, L"SHA-256");
                AddCommand(CommandID::HashAll, L"*");
            }

            if (!HasItems)
            {
                ::DestroyMenu(SubMenu);
                return MAKE_HRESULT(SEVERITY_SUCCESS, 0, 0);
            }

            ::InsertMenuW(
                hMenu,
                indexMenu,
                MF_POPUP,
                reinterpret_cast<UINT_PTR>(SubMenu),
                L"NanaZip Preview");

            return MAKE_HRESULT(
                SEVERITY_SUCCESS,
                0,
                static_cast<UINT>(this->m_commands.size()));
        }

        HRESULT STDMETHODCALLTYPE InvokeCommand(
            _In_ LPCMINVOKECOMMANDINFO lpici) override
        {
            if (!lpici)
            {
                return E_INVALIDARG;
            }

            if (HIWORD(lpici->lpVerb))
            {
                return E_INVALIDARG;
            }

            UINT CommandID = LOWORD(lpici->lpVerb);
            auto It = this->m_commands.find(CommandID);
            if (It == this->m_commands.end())
            {
                return E_INVALIDARG;
            }

            try
            {
                return winrt::make<ExplorerCommandBase>(
                    std::wstring(),
                    It->second)->Invoke(
                        this->m_shellItemArray.get(),
                        nullptr);
            }
            catch (...)
            {
                return winrt::to_hresult();
            }
        }

        HRESULT STDMETHODCALLTYPE GetCommandString(
            _In_ UINT_PTR idCmd,
            _In_ UINT uType,
            _In_opt_ UINT* pReserved,
            _Inout_opt_ LPSTR pszName,
            _In_ UINT cchMax) override
        {
            UNREFERENCED_PARAMETER(pReserved);

            auto It = this->m_commands.find(
                static_cast<UINT>(idCmd));
            if (It == this->m_commands.end())
            {
                return E_INVALIDARG;
            }

            UString Text = GetCommandTitle(It->second);
            if (Text.Len() == 0)
            {
                return E_NOTIMPL;
            }

            if (uType == GCS_VERBW)
            {
                if (pszName && cchMax > 0)
                {
                    wcscpy_s(
                        reinterpret_cast<wchar_t*>(pszName),
                        cchMax,
                        Text.Ptr());
                    return S_OK;
                }
            }
            else if (uType == GCS_VERBA)
            {
                if (pszName && cchMax > 0)
                {
                    std::string Ansi(Text.Len() + 1, '\0');
                    int Converted = ::WideCharToMultiByte(
                        CP_ACP,
                        0,
                        Text.Ptr(),
                        static_cast<int>(Text.Len()),
                        Ansi.data(),
                        static_cast<int>(Text.Len() + 1),
                        nullptr,
                        nullptr);
                    if (Converted >= 0 &&
                        static_cast<UINT>(Converted) < cchMax)
                    {
                        strcpy_s(
                            pszName,
                            cchMax,
                            Ansi.c_str());
                        return S_OK;
                    }
                }
            }

            return E_NOTIMPL;
        }

        HRESULT STDMETHODCALLTYPE Initialize(
            _In_opt_ PCIDLIST_ABSOLUTE pidlFolder,
            _In_opt_ IDataObject* pdtobj,
            _In_opt_ HKEY hkeyProgID) override
        {
            UNREFERENCED_PARAMETER(pidlFolder);
            UNREFERENCED_PARAMETER(hkeyProgID);

            if (!pdtobj)
            {
                return E_INVALIDARG;
            }

            HRESULT Result = ::SHCreateShellItemArrayFromDataObject(
                pdtobj,
                IID_PPV_ARGS(this->m_shellItemArray.put()));
            if (SUCCEEDED(Result))
            {
                return Result;
            }

            // Explorer's IDataObject may only expose CF_HDROP (7-Zip's
            // GetFileNames uses DragQueryFile), so fall back to CF_HDROP
            // when SHCreateShellItemArrayFromDataObject fails.
            FORMATETC FormatEtc
            {
                CF_HDROP,
                nullptr,
                DVASPECT_CONTENT,
                -1,
                TYMED_HGLOBAL
            };
            STGMEDIUM StorageMedium{};
            Result = pdtobj->GetData(&FormatEtc, &StorageMedium);
            if (FAILED(Result) || !StorageMedium.hGlobal)
            {
                return Result;
            }

            HDROP Drop = static_cast<HDROP>(
                ::GlobalLock(StorageMedium.hGlobal));
            if (!Drop)
            {
                ::ReleaseStgMedium(&StorageMedium);
                return E_FAIL;
            }

            UINT Count = ::DragQueryFileW(
                Drop,
                0xFFFFFFFF,
                nullptr,
                0);
            std::vector<std::wstring> FilePaths;
            FilePaths.reserve(Count);
            for (UINT i = 0; i < Count; ++i)
            {
                UINT Length = ::DragQueryFileW(
                    Drop,
                    i,
                    nullptr,
                    0);
                std::wstring FilePath(Length + 1, L'\0');
                ::DragQueryFileW(
                    Drop,
                    i,
                    FilePath.data(),
                    Length + 1);
                FilePath.resize(Length);
                FilePaths.push_back(FilePath);
            }
            ::GlobalUnlock(StorageMedium.hGlobal);
            ::ReleaseStgMedium(&StorageMedium);

            if (FilePaths.empty())
            {
                return E_FAIL;
            }

            std::vector<IShellItem*> Items;
            Items.reserve(FilePaths.size());
            for (std::wstring const& FilePath : FilePaths)
            {
                IShellItem* Item = nullptr;
                if (SUCCEEDED(::SHCreateItemFromParsingName(
                    FilePath.c_str(),
                    nullptr,
                    IID_PPV_ARGS(&Item))))
                {
                    Items.push_back(Item);
                }
            }

            if (Items.empty())
            {
                return E_FAIL;
            }

            Result = ::SHCreateShellItemArray(
                static_cast<DWORD>(Items.size()),
                Items.data(),
                IID_PPV_ARGS(this->m_shellItemArray.put()));

            for (IShellItem* Item : Items)
            {
                Item->Release();
            }

            return Result;
        }
    };

    class DECLSPEC_UUID("EBE9FFD4-AFB4-4395-916E-005D98E97082")
        ClassFactoryContextMenu final : public IClassFactory
    {
    public:

        ClassFactoryContextMenu() : m_ref(1)
        {
        }

        ClassFactoryContextMenu(ClassFactoryContextMenu const&) = delete;
        ClassFactoryContextMenu& operator=(
            ClassFactoryContextMenu const&) = delete;

        HRESULT STDMETHODCALLTYPE QueryInterface(
            _In_ REFIID riid,
            _COM_Outptr_ void** ppvObject) override
        {
            if (!ppvObject)
            {
                return E_POINTER;
            }

            *ppvObject = nullptr;

            if (riid != IID_IUnknown && riid != IID_IClassFactory)
            {
                return E_NOINTERFACE;
            }

            *ppvObject = static_cast<IClassFactory*>(this);
            this->AddRef();
            return S_OK;
        }

        ULONG STDMETHODCALLTYPE AddRef() override
        {
            return static_cast<ULONG>(::InterlockedIncrement(&this->m_ref));
        }

        ULONG STDMETHODCALLTYPE Release() override
        {
            ULONG RefCount = static_cast<ULONG>(
                ::InterlockedDecrement(&this->m_ref));
            if (!RefCount)
            {
                delete this;
            }

            return RefCount;
        }

        HRESULT STDMETHODCALLTYPE CreateInstance(
            _In_opt_ IUnknown* pUnkOuter,
            _In_ REFIID riid,
            _COM_Outptr_ void** ppvObject) override
        {
            if (pUnkOuter)
            {
                return CLASS_E_NOAGGREGATION;
            }

            NanaZipContextMenu* Instance =
                new (std::nothrow) NanaZipContextMenu();
            if (!Instance)
            {
                return E_OUTOFMEMORY;
            }

            HRESULT Result = Instance->QueryInterface(riid, ppvObject);
            Instance->Release();
            return Result;
        }

        HRESULT STDMETHODCALLTYPE LockServer(
            _In_ BOOL fLock) override
        {
            if (fLock)
            {
                ::InterlockedIncrement(&::g_DllRefCount);
            }
            else
            {
                ::InterlockedDecrement(&::g_DllRefCount);
            }

            return S_OK;
        }

    private:

        ~ClassFactoryContextMenu() = default;

        LONG m_ref;
    };

    struct DECLSPEC_UUID("469D94E9-6AF4-4395-B396-99B1308F8CE5")
        ClassFactory : public winrt::implements<
        ClassFactory, IClassFactory>
    {
    public:

        HRESULT STDMETHODCALLTYPE CreateInstance(
            _In_opt_ IUnknown* pUnkOuter,
            _In_ REFIID riid,
            _COM_Outptr_ void** ppvObject) noexcept override
        {
            UNREFERENCED_PARAMETER(pUnkOuter);

            try
            {
                return winrt::make<ExplorerCommandRoot>()->QueryInterface(
                    riid, ppvObject);
            }
            catch (...)
            {
                return winrt::to_hresult();
            }
        }

        HRESULT STDMETHODCALLTYPE LockServer(
            _In_ BOOL fLock) noexcept override
        {
            if (fLock)
            {
                ++winrt::get_module_lock();
            }
            else
            {
                --winrt::get_module_lock();
            }

            return S_OK;
        }
    };
}

EXTERN_C HRESULT STDAPICALLTYPE DllCanUnloadNow()
{
    if (winrt::get_module_lock() || ::g_DllRefCount)
    {
        return S_FALSE;
    }

    winrt::clear_factory_cache();
    return S_OK;
}

EXTERN_C HRESULT STDAPICALLTYPE DllGetClassObject(
    _In_ REFCLSID rclsid,
    _In_ REFIID riid,
    _Outptr_ LPVOID* ppv)
{
    if (!ppv)
    {
        return E_POINTER;
    }

    if (riid != IID_IClassFactory && riid != IID_IUnknown)
    {
        return E_NOINTERFACE;
    }

    if (rclsid != __uuidof(NanaZip::ShellExtension::ClassFactory) &&
        rclsid != __uuidof(NanaZip::ShellExtension::ClassFactoryContextMenu))
    {
        return E_INVALIDARG;
    }

    try
    {
        if (rclsid ==
            __uuidof(NanaZip::ShellExtension::ClassFactoryContextMenu))
        {
            NanaZip::ShellExtension::ClassFactoryContextMenu* Factory =
                new (std::nothrow)
                    NanaZip::ShellExtension::ClassFactoryContextMenu();
            if (!Factory)
            {
                return E_OUTOFMEMORY;
            }

            HRESULT Result = Factory->QueryInterface(riid, ppv);
            Factory->Release();
            return Result;
        }
        return winrt::make<NanaZip::ShellExtension::ClassFactory>(
            )->QueryInterface(riid, ppv);
    }
    catch (...)
    {
        return winrt::to_hresult();
    }
}

long g_DllRefCount = 0;
HWND g_HWND = nullptr;
HINSTANCE g_hInstance = nullptr;

// Only used in Shell Extension to check whether NanaZip Modern is loaded.
EXTERN_C HMODULE K7ModernCurrentModule;

BOOL WINAPI DllMain(
    _In_ HINSTANCE hinstDLL,
    _In_ DWORD fdwReason,
    _In_ LPVOID lpvReserved)
{
    UNREFERENCED_PARAMETER(lpvReserved);

    switch (fdwReason)
    {
    case DLL_PROCESS_ATTACH:
    {
        g_hInstance = hinstDLL;
        ::K7ModernCurrentModule = hinstDLL;
        break;
    }
    case DLL_THREAD_ATTACH:
        break;
    case DLL_THREAD_DETACH:
        break;
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}
