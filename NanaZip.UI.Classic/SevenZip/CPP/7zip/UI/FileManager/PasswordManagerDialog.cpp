// PasswordManagerDialog.cpp

#include "StdAfx.h"

// **************** NanaZip Modification Start ****************
#include "PasswordManagerDialog.h"

#include "../../../Common/IntToString.h"

#include "PasswordMasterDialog.h"
#include "PasswordStore.h"

using namespace NWindows;

bool CPasswordManagerDialog::EnsureUnlocked()
{
  if (NPasswordStore::IsUnlocked())
    return true;
  // Force the store state to be loaded before checking the lock state.
  UStringVector loaded;
  NPasswordStore::GetPasswords(loaded);
  if (NPasswordStore::IsUnlocked())
    return true;
  if (!NPasswordStore::HasMasterPassword())
    return true;
  for (;;)
  {
    CPasswordMasterDialog dialog;
    dialog.Mode = CPasswordMasterDialog::kUnlock;
    if (dialog.Create(*this) != IDOK)
      return false;
    if (NPasswordStore::Unlock(dialog.Password))
      return true;
    ::MessageBoxW(*this, L"Wrong master password.", L"NanaZip", MB_ICONWARNING);
  }
}

void CPasswordManagerDialog::ReloadList()
{
  _list.DeleteAllItems();
  NPasswordStore::GetPasswords(_items);
  int index = 0;
  for (unsigned i = 0; i < _items.Size(); i++)
  {
    wchar_t sz[16];
    ConvertUInt32ToString((UInt32)(i + 1), sz);
    _list.InsertItem(index, sz);
    UString shown;
    if (_showPlain)
      shown = _items[i];
    else
      for (unsigned j = 0; j < _items[i].Len(); j++)
        shown += L'*';
    _list.SetSubItem(index, 1, shown);
    index++;
  }
}

bool CPasswordManagerDialog::OnInit()
{
  if (!EnsureUnlocked())
  {
    End(IDCANCEL);
    return false;
  }

  _list.Attach(GetItem(IDL_PM_LIST));
  _list.SetUnicodeFormat();

  _list.InsertColumn(0, L"#", 30);
  _list.InsertColumn(1, L"Password", 380);

  CheckButton(IDX_PM_SHOWPLAIN, _showPlain);

  ReloadList();

  _list.SetColumnWidthAuto(0);
  _list.SetColumnWidthAuto(1);

  NormalizeSize();
  return CModalDialog::OnInit();
}

bool CPasswordManagerDialog::OnSize(WPARAM /* wParam */, int xSize, int ySize)
{
  int mx, my;
  GetMargins(8, mx, my);
  int bx, by;
  GetItemSizes(IDOK, bx, by);
  int y = ySize - my - by;
  int x = xSize - mx - bx;

  InvalidateRect(NULL);

  MoveItem(IDOK, x, y, bx, by);
  MoveItem(IDB_PM_LOCK, x - mx - bx, y, bx, by);
  MoveItem(IDB_PM_CHANGEMASTER, x - 2 * (mx + bx), y, bx, by);
  MoveItem(IDB_PM_CLEARALL, x - 3 * (mx + bx), y, bx, by);
  MoveItem(IDB_PM_REMOVE, x - 4 * (mx + bx), y, bx, by);
  MoveItem(IDX_PM_SHOWPLAIN, mx, y, x - 5 * (mx + bx), by);
  _list.Move(mx, my, xSize - mx * 2, y - my * 2);
  return false;
}

void CPasswordManagerDialog::OnOK()
{
  CModalDialog::OnOK();
}

bool CPasswordManagerDialog::OnButtonClicked(int buttonID, HWND buttonHWND)
{
  if (buttonID == IDX_PM_SHOWPLAIN)
  {
    _showPlain = IsButtonCheckedBool(IDX_PM_SHOWPLAIN);
    ReloadList();
    return true;
  }
  if (buttonID == IDB_PM_REMOVE)
  {
    for (;;)
    {
      const int index = _list.GetNextSelectedItem(-1);
      if (index < 0 || (unsigned)index >= _items.Size())
        break;
      if (!NPasswordStore::RemovePassword((unsigned)index))
        break;
      _items.Delete((unsigned)index);
      _list.DeleteItem(index);
    }
    return true;
  }
  if (buttonID == IDB_PM_CLEARALL)
  {
    if (::MessageBoxW(*this, L"Delete all saved passwords?", L"NanaZip",
          MB_OKCANCEL | MB_ICONWARNING) == IDOK)
    {
      if (NPasswordStore::ClearPasswords())
        ReloadList();
    }
    return true;
  }
  if (buttonID == IDB_PM_CHANGEMASTER)
  {
    CPasswordMasterDialog dialog;
    dialog.Mode = CPasswordMasterDialog::kSet;
    if (dialog.Create(*this) == IDOK)
    {
      if (!NPasswordStore::ChangeMasterPassword(dialog.Password))
        ::MessageBoxW(*this, L"Cannot write the password store.", L"NanaZip", MB_ICONWARNING);
    }
    return true;
  }
  if (buttonID == IDB_PM_LOCK)
  {
    NPasswordStore::Lock();
    End(IDOK);
    return true;
  }
  return CDialog::OnButtonClicked(buttonID, buttonHWND);
}
// **************** NanaZip Modification End ****************
