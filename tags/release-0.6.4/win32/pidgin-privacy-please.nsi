;NSIS Modern User Interface
;Basic Example Script
;Written by Joost Verburg

;--------------------------------
;Include Modern UI

!include "MUI.nsh"

;--------------------------------
;General

;Name and file
Name "Pidgin Privacy Please Plugin"
OutFile "pidgin-privacy-please-${VERSION}.exe"

;Default installation folder
InstallDir "$PROGRAMFILES\Pidgin"

;Get installation folder from registry if available
;TODO: re-enable this at >0.6.4 (had to be disabled as a work-around because
;older versions might have stored Pidgin\plugins in there
;InstallDirRegKey HKCU "Software\Pidgin Privacy Please Plugin" ""

;Abort if some files cannot be written
AllowSkipFiles off

;Request application privileges for Windows Vista
RequestExecutionLevel highest

;--------------------------------
;Interface Settings

!define MUI_ABORTWARNING

;--------------------------------
;Pages

;!insertmacro MUI_PAGE_LICENSE "Basic.nsi"
;!insertmacro MUI_PAGE_COMPONENTS
!insertmacro MUI_PAGE_DIRECTORY
!insertmacro MUI_PAGE_INSTFILES

!insertmacro MUI_UNPAGE_CONFIRM
!insertmacro MUI_UNPAGE_INSTFILES

;--------------------------------
;Other stuff
!define UNINSTALL_KEY "Software\Microsoft\Windows\CurrentVersion\Uninstall\pidgin-privacy-please"

;--------------------------------
;Languages

!insertmacro MUI_LANGUAGE "English"

;--------------------------------
;Installer Sections

Section "PidginPrivacyPlease" SecPidginPP

SetOutPath "$INSTDIR\plugins"

File "..\src\pidgin-pp.dll"

!include po_install.nsi

;Store installation folder
WriteRegStr HKCU "Software\Pidgin Privacy Please Plugin" "" "$INSTDIR"

;Add uninstall information to the registry
WriteRegStr HKLM "${UNINSTALL_KEY}" "DisplayName" "Pidgin Privacy Please Plugin"
WriteRegStr HKLM "${UNINSTALL_KEY}" "UninstallString" "$INSTDIR\Uninstall-pidgin-pp.exe"
WriteRegStr HKLM "${UNINSTALL_KEY}" "HelpLink" "http://pidgin-privacy-please.googlecode.com/"
WriteRegStr HKLM "${UNINSTALL_KEY}" "DisplayVersion" "${VERSION}"
WriteRegStr HKLM "${UNINSTALL_KEY}" "Publisher" "Stefan Ott"
WriteRegDWORD HKLM "${UNINSTALL_KEY}" "NoModify" 1
WriteRegDWORD HKLM "${UNINSTALL_KEY}" "NoRepair" 1

;Create uninstaller
WriteUninstaller "$INSTDIR\Uninstall-pidgin-pp.exe"

SectionEnd

;--------------------------------
;Descriptions

;Language strings
LangString DESC_SecPidginPP ${LANG_ENGLISH} "The Pidgin Privacy Please Plugin."

;Assign language strings to sections
;!insertmacro MUI_FUNCTION_DESCRIPTION_BEGIN
;	!insertmacro MUI_DESCRIPTION_TEXT ${SecPidginPP} $(DESC_SecPidginPP)
;!insertmacro MUI_FUNCTION_DESCRIPTION_END

;--------------------------------
;Uninstaller Section

Section "Uninstall"

Delete "$INSTDIR\Uninstall-pidgin-pp.exe"
Delete "$INSTDIR\plugins\pidgin-pp.dll"

!include po_uninstall.nsi

RMDir "$INSTDIR\locale"

DeleteRegKey /ifempty HKCU "Software\Pidgin Privacy Please Plugin"
DeleteRegKey HKLM "${UNINSTALL_KEY}"

SectionEnd
