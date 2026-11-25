// OptionsDialog.cpp

#include "StdAfx.h"

#include "../../../Windows/Control/Dialog.h"
#include "../../../Windows/Control/PropertyPage.h"

#include "DialogSize.h"

#include "EditPage.h"
#include "EditPageRes.h"
#include "FileTypesPage.h"
#include "FileTypesPageRes.h"
#include "FoldersPage.h"
#include "FoldersPageRes.h"
#include "MenuPage.h"
#include "MenuPageRes.h"
#include "SettingsPage.h"
#include "SettingsPageRes.h"

#include "App.h"
#include "LangUtils.h"
#include "MyLoadMenu.h"

#include "resource.h"

using namespace NWindows;

void OptionsDialog(HWND hwndOwner, HINSTANCE hInstance);
void OptionsDialog(HWND hwndOwner, HINSTANCE /* hInstance */)
{
  CMenuPage menuPage;
  CFoldersPage foldersPage;
  CEditPage editPage;
  CSettingsPage settingsPage;
  CFileTypesPage fileTypesPage;

  CObjectVector<NControl::CPageInfo> pages;
  BIG_DIALOG_SIZE(200, 200);

  const UINT pageIDs[] = {
      SIZED_DIALOG(IDD_MENU),
      SIZED_DIALOG(IDD_FOLDERS),
      SIZED_DIALOG(IDD_EDIT),
      SIZED_DIALOG(IDD_SETTINGS),
      SIZED_DIALOG(IDD_FILETYPES)
  };

  // The page titles are provided by the external language file. These
  // defaults are used while the language file has no entry for the page.
  static const char * const kDefaultPageTitles[] =
  {
      "Integration"
    , "Folders"
    , "Editor"
    , "Settings"
    , "File Types"
  };

  NControl::CPropertyPage *pagePointers[] = { &menuPage, &foldersPage, &editPage, &settingsPage, &fileTypesPage };

  for (unsigned i = 0; i < ARRAY_SIZE(pageIDs); i++)
  {
    NControl::CPageInfo &page = pages.AddNew();
    page.ID = pageIDs[i];
    LangString_OnlyFromLangFile(page.ID, page.Title);
    if (page.Title.IsEmpty())
      page.Title = kDefaultPageTitles[i];
    page.Page = pagePointers[i];
  }

  INT_PTR res = NControl::MyPropertySheet(pages, hwndOwner, LangString(IDS_OPTIONS));

  if (res != -1 && res != 0)
  {
    g_App.SetListSettings();
    g_App.RefreshAllPanels();
    // ::PostMessage(hwndOwner, kLangWasChangedMessage, 0 , 0);
  }
}
