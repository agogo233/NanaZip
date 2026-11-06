#define AppName "NanaZip"
#define AppPublisher "M2-Team"
#define AppCopyright "© M2-Team and Contributors. All rights reserved."
#define AppURL "https://github.com/M2Team/NanaZip"

#ifndef AppVersion
#define AppVersion "7.0.0.0"
#endif

[Setup]
AppId={{BCA80DD5-FE6C-4104-B629-F09D45E7525C}
AppName={#AppName}
AppCopyright={#AppCopyright}
AppVersion={#AppVersion}
AppPublisher={#AppPublisher}
AppPublisherURL={#AppURL}
AppSupportURL={#AppURL}
AppUpdatesURL={#AppURL}
DefaultDirName={autopf}\{#AppName}
DisableDirPage=yes
DefaultGroupName={#AppName}
DisableProgramGroupPage=yes
VersionInfoVersion={#AppVersion}
UninstallDisplayName={#AppName}

PrivilegesRequired=lowest
OutputBaseFilename=NanaZipSetup_{#AppVersion}_x64
SolidCompression=yes
WizardStyle=modern

ArchitecturesAllowed=x64os
ArchitecturesInstallIn64BitMode=x64os

[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"

[Files]
Source: "..\Output\Binaries\Root\Binaries\x64\*"; DestDir: "{app}"; Flags: ignoreversion restartreplace uninsrestartdelete
Source: "..\Output\Binaries\Root\Binaries\*.txt"; DestDir: "{app}"; Flags: ignoreversion
Source: "..\Output\Binaries\Release\NanaZipPackage\x64\NanaZip.ShellExtension.dll"; DestDir: "{app}"; Flags: ignoreversion restartreplace uninsrestartdelete

[Registry]
; Register the NanaZip shell extension COM server (CLSID mirrors the MSIX
; com:Class declaration in NanaZipPackage\Package.appxmanifest).
Root: HKCU; Subkey: "Software\Classes\CLSID\{{469D94E9-6AF4-4395-B396-99B1308F8CE5}"; ValueType: string; ValueData: "NanaZip Shell Extension"; Flags: uninsdeletekey
Root: HKCU; Subkey: "Software\Classes\CLSID\{{469D94E9-6AF4-4395-B396-99B1308F8CE5}\InprocServer32"; ValueType: string; ValueData: "{app}\NanaZip.ShellExtension.dll"; Flags: uninsdeletekey
Root: HKCU; Subkey: "Software\Classes\CLSID\{{469D94E9-6AF4-4395-B396-99B1308F8CE5}\InprocServer32"; ValueType: string; ValueName: "ThreadingModel"; ValueData: "Apartment"; Flags: uninsdeletekey
; Register the classic IContextMenu shell extension CLSID
Root: HKCU; Subkey: "Software\Classes\CLSID\{{EBE9FFD4-AFB4-4395-916E-005D98E97082}"; ValueType: string; ValueData: "NanaZip Shell Extension"; Flags: uninsdeletekey
Root: HKCU; Subkey: "Software\Classes\CLSID\{{EBE9FFD4-AFB4-4395-916E-005D98E97082}\InprocServer32"; ValueType: string; ValueData: "{app}\NanaZip.ShellExtension.dll"; Flags: uninsdeletekey
Root: HKCU; Subkey: "Software\Classes\CLSID\{{EBE9FFD4-AFB4-4395-916E-005D98E97082}\InprocServer32"; ValueType: string; ValueName: "ThreadingModel"; ValueData: "Apartment"; Flags: uninsdeletekey

; Register the explorer context menu for files, directories and drives
; (mirrors the extension categories declared in NanaZipPackage\
; Package.appxmanifest). The menu entries show in the classic context menu:
; directly on Windows 10, and under "Show more options" on Windows 11,
; because the modern Windows 11 menu requires package identity.
Root: HKCU; Subkey: "Software\Classes\*\shellex\ContextMenuHandlers\NanaZip"; ValueType: string; ValueData: "{{EBE9FFD4-AFB4-4395-916E-005D98E97082}"; Flags: uninsdeletekey
Root: HKCU; Subkey: "Software\Classes\Directory\shellex\ContextMenuHandlers\NanaZip"; ValueType: string; ValueData: "{{EBE9FFD4-AFB4-4395-916E-005D98E97082}"; Flags: uninsdeletekey
Root: HKCU; Subkey: "Software\Classes\Drive\shellex\ContextMenuHandlers\NanaZip"; ValueType: string; ValueData: "{{EBE9FFD4-AFB4-4395-916E-005D98E97082}"; Flags: uninsdeletekey

; Register the NanaZip archive ProgId (mirrors the uap3:FileTypeAssociation
; declaration in NanaZipPackage\Package.appxmanifest).
Root: HKCU; Subkey: "Software\Classes\NanaZip.Archive"; ValueType: string; ValueData: "NanaZip Archive"; Flags: uninsdeletekey
Root: HKCU; Subkey: "Software\Classes\NanaZip.Archive\DefaultIcon"; ValueType: string; ValueData: "{app}\NanaZip.Modern.FileManager.exe,0"; Flags: uninsdeletekey
Root: HKCU; Subkey: "Software\Classes\NanaZip.Archive\shell\open\command"; ValueType: string; ValueData: """{app}\NanaZip.Modern.FileManager.exe"" ""%1"" -open"; Flags: uninsdeletekey

; Offer NanaZip in the "Open with" dialog for every supported archive format.
Root: HKCU; Subkey: "Software\Classes\.001\OpenWithProgids"; ValueType: string; ValueName: "NanaZip.Archive"; ValueData: ""; Flags: uninsdeletevalue
Root: HKCU; Subkey: "Software\Classes\.7z\OpenWithProgids"; ValueType: string; ValueName: "NanaZip.Archive"; ValueData: ""; Flags: uninsdeletevalue
Root: HKCU; Subkey: "Software\Classes\.apfs\OpenWithProgids"; ValueType: string; ValueName: "NanaZip.Archive"; ValueData: ""; Flags: uninsdeletevalue
Root: HKCU; Subkey: "Software\Classes\.arj\OpenWithProgids"; ValueType: string; ValueName: "NanaZip.Archive"; ValueData: ""; Flags: uninsdeletevalue
Root: HKCU; Subkey: "Software\Classes\.asar\OpenWithProgids"; ValueType: string; ValueName: "NanaZip.Archive"; ValueData: ""; Flags: uninsdeletevalue
Root: HKCU; Subkey: "Software\Classes\.b64\OpenWithProgids"; ValueType: string; ValueName: "NanaZip.Archive"; ValueData: ""; Flags: uninsdeletevalue
Root: HKCU; Subkey: "Software\Classes\.br\OpenWithProgids"; ValueType: string; ValueName: "NanaZip.Archive"; ValueData: ""; Flags: uninsdeletevalue
Root: HKCU; Subkey: "Software\Classes\.bz2\OpenWithProgids"; ValueType: string; ValueName: "NanaZip.Archive"; ValueData: ""; Flags: uninsdeletevalue
Root: HKCU; Subkey: "Software\Classes\.bzip2\OpenWithProgids"; ValueType: string; ValueName: "NanaZip.Archive"; ValueData: ""; Flags: uninsdeletevalue
Root: HKCU; Subkey: "Software\Classes\.cab\OpenWithProgids"; ValueType: string; ValueName: "NanaZip.Archive"; ValueData: ""; Flags: uninsdeletevalue
Root: HKCU; Subkey: "Software\Classes\.cb7\OpenWithProgids"; ValueType: string; ValueName: "NanaZip.Archive"; ValueData: ""; Flags: uninsdeletevalue
Root: HKCU; Subkey: "Software\Classes\.cbr\OpenWithProgids"; ValueType: string; ValueName: "NanaZip.Archive"; ValueData: ""; Flags: uninsdeletevalue
Root: HKCU; Subkey: "Software\Classes\.cbz\OpenWithProgids"; ValueType: string; ValueName: "NanaZip.Archive"; ValueData: ""; Flags: uninsdeletevalue
Root: HKCU; Subkey: "Software\Classes\.cpio\OpenWithProgids"; ValueType: string; ValueName: "NanaZip.Archive"; ValueData: ""; Flags: uninsdeletevalue
Root: HKCU; Subkey: "Software\Classes\.deb\OpenWithProgids"; ValueType: string; ValueName: "NanaZip.Archive"; ValueData: ""; Flags: uninsdeletevalue
Root: HKCU; Subkey: "Software\Classes\.dmg\OpenWithProgids"; ValueType: string; ValueName: "NanaZip.Archive"; ValueData: ""; Flags: uninsdeletevalue
Root: HKCU; Subkey: "Software\Classes\.esd\OpenWithProgids"; ValueType: string; ValueName: "NanaZip.Archive"; ValueData: ""; Flags: uninsdeletevalue
Root: HKCU; Subkey: "Software\Classes\.fat\OpenWithProgids"; ValueType: string; ValueName: "NanaZip.Archive"; ValueData: ""; Flags: uninsdeletevalue
Root: HKCU; Subkey: "Software\Classes\.gz\OpenWithProgids"; ValueType: string; ValueName: "NanaZip.Archive"; ValueData: ""; Flags: uninsdeletevalue
Root: HKCU; Subkey: "Software\Classes\.gzip\OpenWithProgids"; ValueType: string; ValueName: "NanaZip.Archive"; ValueData: ""; Flags: uninsdeletevalue
Root: HKCU; Subkey: "Software\Classes\.hfs\OpenWithProgids"; ValueType: string; ValueName: "NanaZip.Archive"; ValueData: ""; Flags: uninsdeletevalue
Root: HKCU; Subkey: "Software\Classes\.hfsx\OpenWithProgids"; ValueType: string; ValueName: "NanaZip.Archive"; ValueData: ""; Flags: uninsdeletevalue
Root: HKCU; Subkey: "Software\Classes\.iso\OpenWithProgids"; ValueType: string; ValueName: "NanaZip.Archive"; ValueData: ""; Flags: uninsdeletevalue
Root: HKCU; Subkey: "Software\Classes\.lha\OpenWithProgids"; ValueType: string; ValueName: "NanaZip.Archive"; ValueData: ""; Flags: uninsdeletevalue
Root: HKCU; Subkey: "Software\Classes\.liz\OpenWithProgids"; ValueType: string; ValueName: "NanaZip.Archive"; ValueData: ""; Flags: uninsdeletevalue
Root: HKCU; Subkey: "Software\Classes\.lz\OpenWithProgids"; ValueType: string; ValueName: "NanaZip.Archive"; ValueData: ""; Flags: uninsdeletevalue
Root: HKCU; Subkey: "Software\Classes\.lz4\OpenWithProgids"; ValueType: string; ValueName: "NanaZip.Archive"; ValueData: ""; Flags: uninsdeletevalue
Root: HKCU; Subkey: "Software\Classes\.lz5\OpenWithProgids"; ValueType: string; ValueName: "NanaZip.Archive"; ValueData: ""; Flags: uninsdeletevalue
Root: HKCU; Subkey: "Software\Classes\.lzh\OpenWithProgids"; ValueType: string; ValueName: "NanaZip.Archive"; ValueData: ""; Flags: uninsdeletevalue
Root: HKCU; Subkey: "Software\Classes\.lzma\OpenWithProgids"; ValueType: string; ValueName: "NanaZip.Archive"; ValueData: ""; Flags: uninsdeletevalue
Root: HKCU; Subkey: "Software\Classes\.lzma2\OpenWithProgids"; ValueType: string; ValueName: "NanaZip.Archive"; ValueData: ""; Flags: uninsdeletevalue
Root: HKCU; Subkey: "Software\Classes\.ntfs\OpenWithProgids"; ValueType: string; ValueName: "NanaZip.Archive"; ValueData: ""; Flags: uninsdeletevalue
Root: HKCU; Subkey: "Software\Classes\.nupkg\OpenWithProgids"; ValueType: string; ValueName: "NanaZip.Archive"; ValueData: ""; Flags: uninsdeletevalue
Root: HKCU; Subkey: "Software\Classes\.rar\OpenWithProgids"; ValueType: string; ValueName: "NanaZip.Archive"; ValueData: ""; Flags: uninsdeletevalue
Root: HKCU; Subkey: "Software\Classes\.rpm\OpenWithProgids"; ValueType: string; ValueName: "NanaZip.Archive"; ValueData: ""; Flags: uninsdeletevalue
Root: HKCU; Subkey: "Software\Classes\.squashfs\OpenWithProgids"; ValueType: string; ValueName: "NanaZip.Archive"; ValueData: ""; Flags: uninsdeletevalue
Root: HKCU; Subkey: "Software\Classes\.swm\OpenWithProgids"; ValueType: string; ValueName: "NanaZip.Archive"; ValueData: ""; Flags: uninsdeletevalue
Root: HKCU; Subkey: "Software\Classes\.tar\OpenWithProgids"; ValueType: string; ValueName: "NanaZip.Archive"; ValueData: ""; Flags: uninsdeletevalue
Root: HKCU; Subkey: "Software\Classes\.taz\OpenWithProgids"; ValueType: string; ValueName: "NanaZip.Archive"; ValueData: ""; Flags: uninsdeletevalue
Root: HKCU; Subkey: "Software\Classes\.tbz\OpenWithProgids"; ValueType: string; ValueName: "NanaZip.Archive"; ValueData: ""; Flags: uninsdeletevalue
Root: HKCU; Subkey: "Software\Classes\.tbz2\OpenWithProgids"; ValueType: string; ValueName: "NanaZip.Archive"; ValueData: ""; Flags: uninsdeletevalue
Root: HKCU; Subkey: "Software\Classes\.tgz\OpenWithProgids"; ValueType: string; ValueName: "NanaZip.Archive"; ValueData: ""; Flags: uninsdeletevalue
Root: HKCU; Subkey: "Software\Classes\.tlz\OpenWithProgids"; ValueType: string; ValueName: "NanaZip.Archive"; ValueData: ""; Flags: uninsdeletevalue
Root: HKCU; Subkey: "Software\Classes\.tlz4\OpenWithProgids"; ValueType: string; ValueName: "NanaZip.Archive"; ValueData: ""; Flags: uninsdeletevalue
Root: HKCU; Subkey: "Software\Classes\.tlz5\OpenWithProgids"; ValueType: string; ValueName: "NanaZip.Archive"; ValueData: ""; Flags: uninsdeletevalue
Root: HKCU; Subkey: "Software\Classes\.tpz\OpenWithProgids"; ValueType: string; ValueName: "NanaZip.Archive"; ValueData: ""; Flags: uninsdeletevalue
Root: HKCU; Subkey: "Software\Classes\.txz\OpenWithProgids"; ValueType: string; ValueName: "NanaZip.Archive"; ValueData: ""; Flags: uninsdeletevalue
Root: HKCU; Subkey: "Software\Classes\.tz\OpenWithProgids"; ValueType: string; ValueName: "NanaZip.Archive"; ValueData: ""; Flags: uninsdeletevalue
Root: HKCU; Subkey: "Software\Classes\.tzst\OpenWithProgids"; ValueType: string; ValueName: "NanaZip.Archive"; ValueData: ""; Flags: uninsdeletevalue
Root: HKCU; Subkey: "Software\Classes\.tzstd\OpenWithProgids"; ValueType: string; ValueName: "NanaZip.Archive"; ValueData: ""; Flags: uninsdeletevalue
Root: HKCU; Subkey: "Software\Classes\.vdi\OpenWithProgids"; ValueType: string; ValueName: "NanaZip.Archive"; ValueData: ""; Flags: uninsdeletevalue
Root: HKCU; Subkey: "Software\Classes\.vhd\OpenWithProgids"; ValueType: string; ValueName: "NanaZip.Archive"; ValueData: ""; Flags: uninsdeletevalue
Root: HKCU; Subkey: "Software\Classes\.vhdx\OpenWithProgids"; ValueType: string; ValueName: "NanaZip.Archive"; ValueData: ""; Flags: uninsdeletevalue
Root: HKCU; Subkey: "Software\Classes\.vmdk\OpenWithProgids"; ValueType: string; ValueName: "NanaZip.Archive"; ValueData: ""; Flags: uninsdeletevalue
Root: HKCU; Subkey: "Software\Classes\.wim\OpenWithProgids"; ValueType: string; ValueName: "NanaZip.Archive"; ValueData: ""; Flags: uninsdeletevalue
Root: HKCU; Subkey: "Software\Classes\.xar\OpenWithProgids"; ValueType: string; ValueName: "NanaZip.Archive"; ValueData: ""; Flags: uninsdeletevalue
Root: HKCU; Subkey: "Software\Classes\.xz\OpenWithProgids"; ValueType: string; ValueName: "NanaZip.Archive"; ValueData: ""; Flags: uninsdeletevalue
Root: HKCU; Subkey: "Software\Classes\.z\OpenWithProgids"; ValueType: string; ValueName: "NanaZip.Archive"; ValueData: ""; Flags: uninsdeletevalue
Root: HKCU; Subkey: "Software\Classes\.zip\OpenWithProgids"; ValueType: string; ValueName: "NanaZip.Archive"; ValueData: ""; Flags: uninsdeletevalue
Root: HKCU; Subkey: "Software\Classes\.zipx\OpenWithProgids"; ValueType: string; ValueName: "NanaZip.Archive"; ValueData: ""; Flags: uninsdeletevalue
Root: HKCU; Subkey: "Software\Classes\.zst\OpenWithProgids"; ValueType: string; ValueName: "NanaZip.Archive"; ValueData: ""; Flags: uninsdeletevalue
Root: HKCU; Subkey: "Software\Classes\.zstd\OpenWithProgids"; ValueType: string; ValueName: "NanaZip.Archive"; ValueData: ""; Flags: uninsdeletevalue

; Register NanaZip in the "Open with" application list.
Root: HKCU; Subkey: "Software\Classes\Applications\NanaZip.Modern.FileManager.exe"; ValueType: string; ValueData: "NanaZip"; Flags: uninsdeletekey
Root: HKCU; Subkey: "Software\Classes\Applications\NanaZip.Modern.FileManager.exe\DefaultIcon"; ValueType: string; ValueData: "{app}\NanaZip.Modern.FileManager.exe,0"; Flags: uninsdeletekey
Root: HKCU; Subkey: "Software\Classes\Applications\NanaZip.Modern.FileManager.exe\shell\open\command"; ValueType: string; ValueData: """{app}\NanaZip.Modern.FileManager.exe"" ""%1"" -open"; Flags: uninsdeletekey

[Icons]
Name: "{autoprograms}\NanaZip"; Filename: "{app}\NanaZip.Modern.FileManager.exe"
