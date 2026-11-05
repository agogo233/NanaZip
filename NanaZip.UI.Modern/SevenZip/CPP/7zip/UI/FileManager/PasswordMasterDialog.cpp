// PasswordMasterDialog.cpp

#include "StdAfx.h"

// **************** NanaZip Modification Start ****************
#include "PasswordMasterDialog.h"

void CPasswordMasterDialog::ReadControls()
{
  _passwordEdit.GetText(Password);
  ShowPassword = IsButtonCheckedBool(IDX_PM_SHOW);
}

void CPasswordMasterDialog::SetTextSpec()
{
  _passwordEdit.SetPasswordChar(ShowPassword ? 0 : TEXT('*'));
  _password2Edit.SetPasswordChar(ShowPassword ? 0 : TEXT('*'));
}

bool CPasswordMasterDialog::OnInit()
{
  if (Mode == kUnlock)
  {
    SetText(L"Unlock password store");
    SetItemText(IDT_PM_INFO,
        L"Enter the master password to unlock the saved archive passwords.");
    ShowItem_Bool(IDT_PM_PASSWORD2, false);
    ShowItem_Bool(IDE_PM_PASSWORD2, false);
  }
  else
  {
    SetText(L"Set master password");
    SetItemText(IDT_PM_INFO,
        L"Protect the saved passwords with a master password. Leave empty "
        L"to use the current Windows user account instead. A lost master "
        L"password cannot be recovered.");
  }
  _passwordEdit.Attach(GetItem(IDE_PM_PASSWORD));
  _password2Edit.Attach(GetItem(IDE_PM_PASSWORD2));
  CheckButton(IDX_PM_SHOW, ShowPassword);
  SetTextSpec();
  return CModalDialog::OnInit();
}

bool CPasswordMasterDialog::OnButtonClicked(int buttonID, HWND buttonHWND)
{
  if (buttonID == IDX_PM_SHOW)
  {
    ReadControls();
    SetTextSpec();
    return true;
  }
  return CDialog::OnButtonClicked(buttonID, buttonHWND);
}

void CPasswordMasterDialog::OnOK()
{
  ReadControls();
  if (Mode == kSet)
  {
    UString password2;
    _password2Edit.GetText(password2);
    if (password2 != Password)
    {
      ::MessageBoxW(*this, L"Passwords do not match.", L"NanaZip", MB_ICONWARNING);
      return;
    }
  }
  CModalDialog::OnOK();
}
// **************** NanaZip Modification End ****************
