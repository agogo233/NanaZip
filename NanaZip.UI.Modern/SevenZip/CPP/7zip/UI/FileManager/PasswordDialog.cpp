// PasswordDialog.cpp

#include "StdAfx.h"

#include "PasswordDialog.h"

#ifdef LANG
#include "LangUtils.h"
#endif

#ifdef LANG
static const UInt32 kLangIDs[] =
{
  IDT_PASSWORD_ENTER,
  IDX_PASSWORD_SHOW
};
#endif

// **************** NanaZip Modification Start ****************
#include "PasswordManagerDialog.h"
#include "PasswordMasterDialog.h"
#include "PasswordStore.h"

namespace NPasswordUI
{

bool RememberPasswordToStore(HWND parent, const UString &password)
{
  if (password.IsEmpty())
    return false;
  // Force the store state to be loaded before checking the lock state.
  UStringVector loaded;
  NPasswordStore::GetPasswords(loaded);
  if (!NPasswordStore::IsUnlocked())
  {
    CPasswordMasterDialog dialog;
    dialog.Mode = CPasswordMasterDialog::kUnlock;
    if (dialog.Create(parent) != IDOK || !NPasswordStore::Unlock(dialog.Password))
      return false;
  }
  return NPasswordStore::AddPassword(password);
}

}
// **************** NanaZip Modification End ****************

// **************** NanaZip Modification Start ****************
void CPasswordDialog::PrefillFromStore()
{
  Password.Empty();
  UStringVector stored;
  if (NPasswordStore::GetPasswords(stored) && !stored.IsEmpty())
    Password = stored[0];
}
// **************** NanaZip Modification End ****************

void CPasswordDialog::ReadControls()
{
  _passwordEdit.GetText(Password);
  ShowPassword = IsButtonCheckedBool(IDX_PASSWORD_SHOW);
  // **************** NanaZip Modification Start ****************
  Remember = IsButtonCheckedBool(IDX_PASSWORD_REMEMBER);
  // **************** NanaZip Modification End ****************
}

void CPasswordDialog::SetTextSpec()
{
  _passwordEdit.SetPasswordChar(ShowPassword ? 0: TEXT('*'));
  _passwordEdit.SetText(Password);
}

bool CPasswordDialog::OnInit()
{
  #ifdef LANG
  LangSetWindowText(*this, IDD_PASSWORD);
  LangSetDlgItems(*this, kLangIDs, ARRAY_SIZE(kLangIDs));
  #endif
  _passwordEdit.Attach(GetItem(IDE_PASSWORD_PASSWORD));
  // **************** NanaZip Modification Start ****************
  if (Password.IsEmpty() && !NoPrefill)
    PrefillFromStore();
  // **************** NanaZip Modification End ****************
  CheckButton(IDX_PASSWORD_SHOW, ShowPassword);
  // **************** NanaZip Modification Start ****************
  CheckButton(IDX_PASSWORD_REMEMBER, Remember);
  // **************** NanaZip Modification End ****************
  SetTextSpec();
  bool res = CModalDialog::OnInit();
  // **************** NanaZip Modification Start ****************
  // Select the prefilled password so it can be replaced by typing.
  _passwordEdit.SendMsg(EM_SETSEL, 0, (LPARAM)-1);
  // **************** NanaZip Modification End ****************
  return res;
}

bool CPasswordDialog::OnButtonClicked(int buttonID, HWND buttonHWND)
{
  if (buttonID == IDX_PASSWORD_SHOW)
  {
    ReadControls();
    SetTextSpec();
    return true;
  }
  // **************** NanaZip Modification Start ****************
  if (buttonID == IDB_PASSWORD_MANAGE)
  {
    ReadControls();
    CPasswordManagerDialog dialog;
    dialog.Create(*this);
    if (Password.IsEmpty())
      PrefillFromStore();
    SetTextSpec();
    _passwordEdit.SendMsg(EM_SETSEL, 0, (LPARAM)-1);
    return true;
  }
  // **************** NanaZip Modification End ****************
  return CDialog::OnButtonClicked(buttonID, buttonHWND);
}

void CPasswordDialog::OnOK()
{
  ReadControls();
  CModalDialog::OnOK();
}
