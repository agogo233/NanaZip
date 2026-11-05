// OpenCallback.h

#ifndef __OPEN_CALLBACK_H
#define __OPEN_CALLBACK_H

#include "../../../Common/MyCom.h"

#include "../../../Windows/FileFind.h"

#include "../../IPassword.h"

#include "../../Archive/IArchive.h"

#ifdef _SFX
#include "ProgressDialog.h"
#else
#include "ProgressDialog2.h"
#endif


class COpenArchiveCallback:
  public IArchiveOpenCallback,
  public IArchiveOpenVolumeCallback,
  public IArchiveOpenSetSubArchiveName,
  public IProgress,
  public ICryptoGetTextPassword,
  public CMyUnknownImp
{
  FString _folderPrefix;
  NWindows::NFile::NFind::CFileInfo _fileInfo;
  // NWindows::NSynchronization::CCriticalSection _criticalSection;
  bool _subArchiveMode;
  UString _subArchiveName;

public:
  bool PasswordIsDefined;
  bool PasswordWasAsked;
  // **************** NanaZip Modification Start ****************
  // Set when a volume file required for opening is missing. The auto
  // retry loop in FileFolderPluginOpen uses it to stop trying further
  // saved passwords (multi-volume archive is incomplete).
  bool VolumeMissing;
  // Set when the auto retry loop already tried all saved passwords and
  // is falling back to the manual dialog: the password dialog must not
  // prefill the known-failed saved password.
  bool NoPrefill;
  // **************** NanaZip Modification End ****************
  UString Password;
  HWND ParentWindow;
  CProgressDialog ProgressDialog;

  MY_UNKNOWN_IMP5(
    IArchiveOpenCallback,
    IArchiveOpenVolumeCallback,
    IArchiveOpenSetSubArchiveName,
    IProgress,
    ICryptoGetTextPassword)

  INTERFACE_IProgress(;)
  INTERFACE_IArchiveOpenCallback(;)
  INTERFACE_IArchiveOpenVolumeCallback(;)

  // ICryptoGetTextPassword
  STDMETHOD(CryptoGetTextPassword)(BSTR *password);

  STDMETHOD(SetSubArchiveName(const wchar_t *name))
  {
    _subArchiveMode = true;
    _subArchiveName = name;
    return S_OK;
  }

  COpenArchiveCallback():
    ParentWindow(0)
  {
    _subArchiveMode = false;
    PasswordIsDefined = false;
    PasswordWasAsked = false;
    // **************** NanaZip Modification Start ****************
    VolumeMissing = false;
    NoPrefill = false;
    // **************** NanaZip Modification End ****************
  }
  /*
  void Init()
  {
    PasswordIsDefined = false;
    _subArchiveMode = false;
  }
  */
  
  HRESULT LoadFileInfo2(const FString &folderPrefix, const FString &fileName)
  {
    _folderPrefix = folderPrefix;
    if (!_fileInfo.Find_FollowLink(_folderPrefix + fileName))
    {
      return GetLastError_noZero_HRESULT();
    }
    return S_OK;
  }

  void ShowMessage(const UInt64 *completed);

  INT_PTR StartProgressDialog(const UString &title, NWindows::CThread &thread)
  {
    return ProgressDialog.Create(title, thread, ParentWindow);
  }
};

#endif
