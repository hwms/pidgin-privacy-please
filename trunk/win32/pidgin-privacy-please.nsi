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
OutFile "pidgin-privacy-please.exe"

;Default installation folder
InstallDir "$PROGRAMFILES\Pidgin\plugins"

;Get installation folder from registry if available
InstallDirRegKey HKCU "Software\Pidgin Privacy Please Plugin" ""

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
;Languages

!insertmacro MUI_LANGUAGE "English"

;--------------------------------
;Installer Sections

Section "PidginPrivacyPlease" SecPidginPP

SetOutPath "$INSTDIR"

File "pidgin-pp.dll"

;ADD YOUR OWN FILES HERE...

;Store installation folder
WriteRegStr HKCU "Software\Pidgin Privacy Please Plugin" "" $INSTDIR

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

;ADD YOUR OWN FILES HERE...

Delete "$INSTDIR\Uninstall-pidgin-pp.exe"
Delete "$INSTDIR\pidgin-pp.dll"

DeleteRegKey /ifempty HKCU "Software\Pidgin Privacy Please Plugin"

SectionEnd
