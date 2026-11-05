// PasswordMasterDialog.h

#ifndef __PASSWORD_MASTER_DIALOG_H
#define __PASSWORD_MASTER_DIALOG_H

// **************** NanaZip Modification Start ****************
#include "../../../Windows/Control/Dialog.h"
#include "../../../Windows/Control/Edit.h"

#include "PasswordMasterDialogRes.h"

class CPasswordMasterDialog: public NWindows::NControl::CModalDialog
{
  NWindows::NControl::CEdit _passwordEdit;
  NWindows::NControl::CEdit _password2Edit;

  virtual void OnOK();
  virtual bool OnInit();
  virtual bool OnButtonClicked(int buttonID, HWND buttonHWND);
  void SetTextSpec();
  void ReadControls();
public:
  enum EMode
  {
    kSet,
    kUnlock
  };

  EMode Mode;
  UString Password;
  bool ShowPassword;

  CPasswordMasterDialog(): Mode(kSet), ShowPassword(false) {}
  INT_PTR Create(HWND parentWindow = 0) { return CModalDialog::Create(IDD_PASSWORD_MASTER, parentWindow); }
};
// **************** NanaZip Modification End ****************

#endif
