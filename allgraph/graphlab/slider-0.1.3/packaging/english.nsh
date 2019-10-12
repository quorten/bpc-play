# English language strings for Slider's NSIS installer.

# Copyright (C) 2013 Andrew Makousky
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

!insertmacro MUI_LANGUAGE "English"
LangString lng_Full ${LNG_ENGLISH} "Full"
LangString lng_Welcome ${LNG_ENGLISH} \
  "Welcome to the ${PRODUCT_NAME} ${VERSION} Setup Wizard"
LangString lng_WelcomeInfo ${LNG_ENGLISH} \
  "This wizard will guide you through the installation of \
${PRODUCT_NAME} ${VERSION}."
LangString lng_Title ${LNG_ENGLISH} "${PRODUCT_NAME} ${VERSION} Setup"
LangString lng_LICENSE_BUTTON   ${LANG_ENGLISH} "Next >"
LangString lng_LICENSE_BOTTOM_TEXT   ${LANG_ENGLISH} "${PRODUCT_NAME} is released under the Simplified BSD License. The license is provided here for information purposes only. $_CLICK"
LangString lng_Core ${LNG_ENGLISH} "${PRODUCT_NAME} Core Files (required)"
LangString lng_CoreProgress ${LNG_ENGLISH} "Installing ${PRODUCT_NAME}..."
LangString lng_GTK ${LNG_ENGLISH} "GTK+ Runtime Environment (required)"
LangString lng_GTKProgress ${LNG_ENGLISH} "Installing GTK+"
LangString lng_StartDeskShortcuts ${LNG_ENGLISH} \
  "Start Menu and Desktop Shortcuts"
LangString lng_StartShortcuts ${LNG_ENGLISH} "Start Menu Shortcuts"
LangString lng_StartShortcutsProgress ${LNG_ENGLISH} \
  "Creating Start Menu Shortcuts..."
LangString lng_DesktopShortcut ${LNG_ENGLISH} "Desktop Shortcut"
LangString lng_DesktopShortcutProgress ${LNG_ENGLISH} \
  "Creating Desktop Shortcut..."
LangString lng_RegistryProgress ${LNG_ENGLISH} "Creating Registry Keys..."
LangString lng_GenerateUninstaller ${LNG_ENGLISH} "Generating Uninstaller..."
LangString lng_UninstallProgress ${LNG_ENGLISH} \
  "Uninstalling ${PRODUCT_NAME}.."
LangString lng_DeleteProgress ${LNG_ENGLISH} "Deleting Files..."
LangString lng_RemoveShortcutsProgress ${LNG_ENGLISH} "Removing Shortcuts..."
LangString lng_DeleteRegistryProgress ${LNG_ENGLISH} \
  "Deleting Registry Keys..."

LangString lng_CoreDesc ${LNG_ENGLISH} \
  "The core files required to use ${PRODUCT_NAME}"
LangString lng_GTKDesc ${LNG_ENGLISH} \
  "A multi-platform GUI toolkit, used by ${PRODUCT_NAME}"
LangString lng_StartDeskShortcutsDesc ${LNG_ENGLISH} \
  "Adds icons to your start menu and/or your desktop for easy access"
LangString lng_StartShortcutsDesc ${LNG_ENGLISH} \
  "Adds shortcuts to your start menu"
LangString lng_DesktopShortcutDesc ${LNG_ENGLISH} \
  "Adds an icon on your desktop"
