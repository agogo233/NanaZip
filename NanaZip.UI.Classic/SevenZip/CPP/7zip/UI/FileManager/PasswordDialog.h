// PasswordDialog.h

#ifndef __PASSWORD_DIALOG_H
#define __PASSWORD_DIALOG_H

// **************** NanaZip Modification Start ****************
#include "../../../Windows/Control/Dialog.h"
#include "../../../Windows/Control/Edit.h"
// **************** NanaZip Modification End ****************

#include "PasswordDialogRes.h"

class CPasswordDialog: public NWindows::NControl::CModalDialog
{
  NWindows::NControl::CEdit _passwordEdit;

  virtual void OnOK();
  virtual bool OnInit();
  virtual bool OnButtonClicked(int buttonID, HWND buttonHWND);
  void SetTextSpec();
  void ReadControls();
  // **************** NanaZip Modification Start ****************
  void PrefillFromStore();
  // **************** NanaZip Modification End ****************
public:
  UString Password;
  bool ShowPassword;
  // **************** NanaZip Modification Start ****************
  bool Remember;
  // Do not prefill the most recently used saved password (set when the
  // auto retry loop already tried all saved passwords and failed).
  bool NoPrefill;
  // **************** NanaZip Modification End ****************

  CPasswordDialog(): ShowPassword(false)
    // **************** NanaZip Modification Start ****************
    , Remember(false)
    , NoPrefill(false)
    // **************** NanaZip Modification End ****************
    {}
  INT_PTR Create(HWND parentWindow = 0) { return CModalDialog::Create(IDD_PASSWORD, parentWindow); }
};

// **************** NanaZip Modification Start ****************
namespace NPasswordUI
{
  // Saves the password to the store if the user checked "Remember password".
  // Asks for the master password to unlock the store when needed. Returns
  // false when the password was not remembered.
  bool RememberPasswordToStore(HWND parent, const UString &password);
}
// **************** NanaZip Modification End ****************

#endif
