// FileTypesPage.cpp

#include "StdAfx.h"

#include "../../../Common/StringConvert.h"
#include "../../../Windows/Registry.h"

#include "FileTypesPage.h"
#include "FileTypesPageRes.h"

#include "LangUtils.h"
#include "RegistryAssociations.h"

#include <K7User.h>

using namespace NWindows;

// The list mirrors the "Open with" extensions registered by the installer
// (NanaZipSetup.iss). Disk image formats are excluded from the one-click
// "set all" action to preserve the default double-click mounting behavior.
static const char * const kFileTypes[] =
{
    "001"
  , "7z"
  , "apfs"
  , "arj"
  , "asar"
  , "b64"
  , "br"
  , "bz2"
  , "bzip2"
  , "cab"
  , "cb7"
  , "cbr"
  , "cbz"
  , "cpio"
  , "deb"
  , "dmg"
  , "esd"
  , "fat"
  , "gz"
  , "gzip"
  , "hfs"
  , "hfsx"
  , "iso"
  , "lha"
  , "liz"
  , "lz"
  , "lz4"
  , "lz5"
  , "lzh"
  , "lzma"
  , "lzma2"
  , "ntfs"
  , "nupkg"
  , "rar"
  , "rpm"
  , "squashfs"
  , "swm"
  , "tar"
  , "taz"
  , "tbz"
  , "tbz2"
  , "tgz"
  , "tlz"
  , "tlz4"
  , "tlz5"
  , "tpz"
  , "txz"
  , "tz"
  , "tzst"
  , "tzstd"
  , "vdi"
  , "vhd"
  , "vhdx"
  , "vmdk"
  , "wim"
  , "xar"
  , "xz"
  , "z"
  , "zip"
  , "zipx"
  , "zst"
  , "zstd"
};

static const char * const kDiskImageTypes[] =
{
    "esd"
  , "iso"
  , "swm"
  , "vdi"
  , "vhd"
  , "vhdx"
  , "vmdk"
  , "wim"
};

static bool IsDiskImageType(const char *ext)
{
  for (unsigned i = 0; i < ARRAY_SIZE(kDiskImageTypes); i++)
    if (strcmp(ext, kDiskImageTypes[i]) == 0)
      return true;
  return false;
}

static bool IsAssociatedWithNanaZip(const char *ext)
{
  // This page manages user-level (HKCU) associations only, so the check
  // must not fall back to the machine-wide (HKLM) values left by the
  // installer of the older builds: otherwise a stale machine-level value
  // would keep the item shown as checked and impossible to clear.
  const UString extStr = GetUnicodeString(ext);
  NRegistryAssoc::CShellExtInfo info;
  if (info.ReadFromRegistry(HKEY_CURRENT_USER, extStr))
    return info.IsIt7Zip();
  return false;
}

static bool GetSelfCommand(UString &command, UString &iconPath)
{
  wchar_t path[MAX_PATH + 2];
  const DWORD len = ::GetModuleFileNameW(NULL, path, MAX_PATH + 1);
  path[MAX_PATH + 1] = 0;
  if (len == 0 || len > MAX_PATH)
    return false;
  iconPath = path;
  command = L"\"";
  command += path;
  command += L"\" \"%1\" -open";
  return true;
}

bool CFileTypesPage::OnInit()
{
  _initMode = true;

  Clear_AssocChanged();

  _listView.Attach(GetItem(IDL_FILETYPES_LIST));

  const UInt32 newFlags = LVS_EX_CHECKBOXES | LVS_EX_FULLROWSELECT;
  _listView.SetExtendedListViewStyle(newFlags, newFlags);

  _listView.InsertColumn(0, L"", 200);

  for (unsigned i = 0; i < ARRAY_SIZE(kFileTypes); i++)
  {
    UString s = L".";
    s += kFileTypes[i];
    const int itemIndex = _listView.InsertItem(i, s);
    _listView.SetCheckState(itemIndex,
        IsAssociatedWithNanaZip(kFileTypes[i]));
  }

  _listView.SetColumnWidthAuto(0);
  _initMode = false;

  return CPropertyPage::OnInit();
}

LONG CFileTypesPage::OnApply()
{
  if (_assoc_Changed)
  {
    UString command;
    UString iconPath;
    if (!GetSelfCommand(command, iconPath))
    {
      Clear_AssocChanged();
      return PSNRET_NOERROR;
    }

    for (unsigned i = 0; i < ARRAY_SIZE(kFileTypes); i++)
    {
      const char *ext = kFileTypes[i];
      const UString extStr = GetUnicodeString(ext);
      const bool checked = _listView.GetCheckState(i) != 0;
      const bool wasAssociated = IsAssociatedWithNanaZip(ext);
      if (checked == wasAssociated)
        continue;
      if (checked)
      {
        UString title = L".";
        title += ext;
        NRegistryAssoc::AddShellExtensionInfo(HKEY_CURRENT_USER,
            extStr, title, command, iconPath, 0);
      }
      else
        NRegistryAssoc::DeleteShellExtensionInfo(HKEY_CURRENT_USER, extStr);
    }

    ::SHChangeNotify(SHCNE_ASSOCCHANGED, SHCNF_IDLIST, NULL, NULL);

    Clear_AssocChanged();
  }

  return PSNRET_NOERROR;
}

bool CFileTypesPage::OnButtonClicked(int buttonID, HWND buttonHWND)
{
  switch (buttonID)
  {
    case IDB_FILETYPES_ALL:
    case IDB_FILETYPES_NONE:
    {
      const bool state = (buttonID == IDB_FILETYPES_ALL);
      for (unsigned i = 0; i < ARRAY_SIZE(kFileTypes); i++)
      {
        if (state && IsDiskImageType(kFileTypes[i]))
          continue;
        _listView.SetCheckState(i, state);
      }
      break;
    }

    case IDB_FILETYPES_SYSTEM:
    {
      ::K7UserModernLaunchDefaultAppsSettings();
      return true;
    }

    default:
      return CPropertyPage::OnButtonClicked(buttonID, buttonHWND);
  }

  Changed();
  return true;
}


bool CFileTypesPage::OnNotify(UINT controlID, LPNMHDR lParam)
{
  if (lParam->hwndFrom == HWND(_listView))
  {
    switch (lParam->code)
    {
      case (LVN_ITEMCHANGED):
        return OnItemChanged((const NMLISTVIEW *)lParam);
    }
  }
  return CPropertyPage::OnNotify(controlID, lParam);
}


bool CFileTypesPage::OnItemChanged(const NMLISTVIEW *info)
{
  if (_initMode)
    return true;
  if ((info->uChanged & LVIF_STATE) != 0)
  {
    UINT oldState = info->uOldState & LVIS_STATEIMAGEMASK;
    UINT newState = info->uNewState & LVIS_STATEIMAGEMASK;
    if (oldState != newState)
    {
      _assoc_Changed = true;
      Changed();
    }
  }
  return true;
}
