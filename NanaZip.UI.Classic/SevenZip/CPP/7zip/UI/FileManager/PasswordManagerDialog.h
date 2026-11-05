// PasswordManagerDialog.h

#ifndef __PASSWORD_MANAGER_DIALOG_H
#define __PASSWORD_MANAGER_DIALOG_H

// **************** NanaZip Modification Start ****************
#include "../../../Common/MyString.h"

#include "../../../Windows/Control/Dialog.h"
#include "../../../Windows/Control/ListView.h"

#include "PasswordManagerDialogRes.h"

class CPasswordManagerDialog: public NWindows::NControl::CModalDialog
{
  NWindows::NControl::CListView _list;

  bool _showPlain;
  UStringVector _items;

  virtual bool OnInit();
  virtual void OnOK();
  virtual bool OnButtonClicked(int buttonID, HWND buttonHWND);
  virtual bool OnSize(WPARAM wParam, int xSize, int ySize);

  void ReloadList();
  bool EnsureUnlocked();
public:
  CPasswordManagerDialog(): _showPlain(false) {}
  INT_PTR Create(HWND parentWindow = 0) { return CModalDialog::Create(IDD_PASSWORD_MANAGER, parentWindow); }
};
// **************** NanaZip Modification End ****************

#endif
