// FileTypesPage.h

#ifndef __FILE_TYPES_PAGE_H
#define __FILE_TYPES_PAGE_H

#include "../../../Windows/Control/PropertyPage.h"
#include "../../../Windows/Control/ListView.h"

class CFileTypesPage: public NWindows::NControl::CPropertyPage
{
  bool _initMode;

  bool _assoc_Changed;

  void Clear_AssocChanged()
  {
    _assoc_Changed = false;
  }

  NWindows::NControl::CListView _listView;

  virtual bool OnInit();
  virtual bool OnNotify(UINT controlID, LPNMHDR lParam);
  virtual bool OnItemChanged(const NMLISTVIEW *info);
  virtual LONG OnApply();
  virtual bool OnButtonClicked(int buttonID, HWND buttonHWND);
public:
};

#endif
