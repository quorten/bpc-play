# Nullsoft Installer script for Slider.

# Copyright (C) 2011, 2012, 2013 Andrew Makousky
# All rights reserved.
# 
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
# 
#   * Redistributions of source code must retain the above copyright
#     notice, this list of conditions and the following disclaimer.
# 
#   * Redistributions in binary form must reproduce the above
#     copyright notice, this list of conditions and the following
#     disclaimer in the documentation and/or other materials provided
#     with the distribution.
# 
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
# "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
# LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
# FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
# COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
# INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
# BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
# LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
# CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
# LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
# ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
# POSSIBILITY OF SUCH DAMAGE.

######################################################################
# Defines

!ifndef VERSION
  !define VERSION "test"
!endif
!define PRODUCT_NAME "Slider Wave Editor"

######################################################################
# MUI Settings

!include "MUI2.nsh"
!define MUI_ABORTWARNING
!define MUI_FINISHPAGE_NOAUTOCLOSE
!define MUI_UNFINISHPAGE_NOAUTOCLOSE
!define MUI_ICON \
  "${NSISDIR}\Contrib\Graphics\Icons\modern-install-full.ico"
!define MUI_UNICON \
  "${NSISDIR}\Contrib\Graphics\Icons\modern-uninstall-full.ico"
!define MUI_HEADERIMAGE
!define MUI_HEADERIMAGE_RIGHT
!define MUI_HEADERIMAGE_BITMAP ..\svgs\install-header.bmp
!define MUI_HEADERIMAGE_UNBITMAP ..\svgs\install-header.bmp
!define MUI_COMPONENTSPAGE_SMALLDESC

!ifdef TEST_INST
SetCompress off
!else
SetCompressor /SOLID lzma
!endif

######################################################################
# Registry and start menu

!define SLIDER_KEY "Software\SliderWaveEditor"
!define UNINSTALL_KEY \
  "Software\Microsoft\Windows\CurrentVersion\Uninstall\SliderWaveEditor"

######################################################################
# Install pages

InstType $(lng_Full)

!define MUI_WELCOMEPAGE_TITLE $(lng_Welcome)
!define MUI_WELCOMEPAGE_TEXT $(lng_WelcomeInfo)
!insertmacro MUI_PAGE_WELCOME

!define MUI_LICENSEPAGE_BUTTON $(lng_LICENSE_BUTTON)
!define MUI_LICENSEPAGE_TEXT_BOTTOM $(lng_LICENSE_BOTTOM_TEXT)
!insertmacro MUI_PAGE_LICENSE "license.rtf"

!insertmacro MUI_PAGE_COMPONENTS
!insertmacro MUI_PAGE_DIRECTORY
!insertmacro MUI_PAGE_INSTFILES

!define MUI_FINISHPAGE_NOREBOOTSUPPORT

######################################################################
# Uninstaller pages

!insertmacro MUI_UNPAGE_CONFIRM
!insertmacro MUI_UNPAGE_INSTFILES
!insertmacro MUI_PAGE_FINISH

######################################################################
# String localization

!include "english.nsh"
!include "spanish.nsh"

######################################################################
# Settings

Name "${PRODUCT_NAME}"
OutFile "slider-${VERSION}-setup.exe"
Caption "${PRODUCT_NAME} ${VERSION} Setup"
InstallDir "$PROGRAMFILES\${PRODUCT_NAME}"
InstallDirRegKey HKLM "${SLIDER_KEY}" "InstallDir"
RequestExecutionLevel admin

######################################################################
# Installer Sections

Section $(lng_Core) SecCore
    DetailPrint $(lng_CoreProgress)
    SectionIn 1 RO

    SetOutPath "$INSTDIR"
    File /r "instdir\*"
SectionEnd

Section $(lng_GTK) SecGtk
    DetailPrint $(lng_GTKProgress)
    SectionIn 1 RO

    # GTK+ dependencies (moved to a folder called dist)
    SetOutPath "$INSTDIR"
    File /r "2.12.10-dist\*"
SectionEnd

SectionGroup $(lng_StartDeskShortcuts) SecShortcuts
    Section $(lng_StartShortcuts) SecShortcutsStartMenu
        DetailPrint $(lng_StartShortcutsProgress)
        SectionIn 1
        SetOutPath "$SMPROGRAMS"
        CreateShortCut "${PRODUCT_NAME}.lnk" "$INSTDIR\bin\slider.exe"
    SectionEnd
    Section $(lng_DesktopShortcut) SecShortcutsDesktop
        DetailPrint $(lng_DesktopShortcutDesc)
        SectionIn 1
        SetOutPath $DESKTOP
        CreateShortCut "${PRODUCT_NAME}.lnk" "$INSTDIR\bin\slider.exe"
    SectionEnd
SectionGroupEnd

Section -post
    DetailPrint $(lng_RegistryProgress)
    SetOutPath "$INSTDIR"
    WriteRegStr HKLM "${SLIDER_KEY}" "InstallDir" "$INSTDIR"

    # register uninstaller
    WriteRegStr HKLM "${UNINSTALL_KEY}" "UninstallString" \
                "$\"$INSTDIR\uninstall.exe$\""
    WriteRegStr HKLM "${UNINSTALL_KEY}" "QuietUninstallString" \
                "$\"$INSTDIR\uninstall.exe$\" /S"
    WriteRegStr HKLM "${UNINSTALL_KEY}" "InstallLocation" "$\"$INSTDIR$\""

    WriteRegStr HKLM "${UNINSTALL_KEY}" "DisplayName" "${PRODUCT_NAME}"
    WriteRegStr HKLM "${UNINSTALL_KEY}" "DisplayIcon" \
                "$INSTDIR\bin\slider.exe,0"
    WriteRegStr HKLM "${UNINSTALL_KEY}" "DisplayVersion" "${VERSION}"

    WriteRegDWORD HKLM "${UNINSTALL_KEY}" "NoModify" "1"
    WriteRegDWORD HKLM "${UNINSTALL_KEY}" "NoRepair" "1"

    DetailPrint $(lng_GenerateUninstaller)
    WriteUninstaller "$INSTDIR\uninstall.exe"
SectionEnd


Section Uninstall
    DetailPrint $(lng_UninstallProgress)

    DetailPrint $(lng_DeleteProgress)
    RMDir /r "$INSTDIR\bin"
    RMDir /r "$INSTDIR\lib"
    RMDir /r "$INSTDIR\share"
    RMDir /r "$INSTDIR\etc"
    Delete "$INSTDIR\uninstall.exe"
    RMDir "$INSTDIR"

    DetailPrint $(lng_RemoveShortcutsProgress)
    Delete "$SMPROGRAMS\${PRODUCT_NAME}.lnk"
    Delete "$DESKTOP\${PRODUCT_NAME}.lnk"

    DetailPrint $(lng_DeleteRegistryProgress)
    DeleteRegKey HKLM "${SLIDER_KEY}"
    DeleteRegKey HKLM "${UNINSTALL_KEY}"
SectionEnd

# Last is the descriptions

!insertmacro MUI_FUNCTION_DESCRIPTION_BEGIN
    !insertmacro MUI_DESCRIPTION_TEXT ${SecCore} $(lng_CoreDesc)
    !insertmacro MUI_DESCRIPTION_TEXT ${SecGtk} $(lng_GTKDesc)
    !insertmacro MUI_DESCRIPTION_TEXT ${SecShortcuts} \
      $(lng_StartDeskShortcutsDesc)
    !insertmacro MUI_DESCRIPTION_TEXT ${SecShortcutsStartMenu} \
      $(lng_StartShortcutsDesc)
    !insertmacro MUI_DESCRIPTION_TEXT ${SecShortcutsDesktop} \
      $(lng_DesktopShortcutDesc)
!insertmacro MUI_FUNCTION_DESCRIPTION_END

######################################################################
# Language Initialization Dialog

# !define LANG_SEL_DIALOG
!ifdef LANG_SEL_DIALOG
Function .onInit
    Push ""
    Push ${LANG_ENGLISH}
    Push English
    Push ${LANG_SPANISH}
    Push Spanish
    Push A # `A' means auto count languages
    # For the auto count to work the first empty push (Push "") must
    # remain intact.

    LangDLL::LangDialog "Installer Language" \
      "Please select the language of the installer"

    Pop $LANGUAGE
    StrCmp $LANGUAGE "cancel" 0 +2
        Abort
FunctionEnd
!endif
