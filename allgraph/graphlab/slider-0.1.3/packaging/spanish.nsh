# Spanish language strings for Slider's NSIS installer.

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

!insertmacro MUI_LANGUAGE "Spanish"
LangString lng_Full ${LNG_SPANISH} "Completa"
LangString lng_Welcome ${LNG_SPANISH} \
  "Welcome to the ${PRODUCT_NAME} ${VERSION} Setup Wizaro"
LangString lng_WelcomeInfo ${LNG_SPANISH} \
  "Ese wizaro will guide you through la instalación de \
${PRODUCT_NAME} ${VERSION}."
LangString lng_Title ${LNG_SPANISH} "${PRODUCT_NAME} ${VERSION} Setup"
LangString lng_LICENSE_BUTTON   ${LANG_SPANISH} "Siguiente >"
LangString lng_LICENSE_BOTTOM_TEXT   ${LANG_SPANISH} "$(^Name) se publica bajo la Licencia BSD Simplica. Esta licencia se muestra aquí solamente como información. $_CLICK"
LangString lng_Core ${LNG_SPANISH} \
  "${PRODUCT_NAME} Archivos básicos (requerido)"
LangString lng_CoreProgress ${LNG_SPANISH} "Instalar ${PRODUCT_NAME}..."
LangString lng_GTK ${LNG_SPANISH} "GTK+ Runtime Environment (requerido)"
LangString lng_GTKProgress ${LNG_SPANISH} "Instalar GTK+"
LangString lng_StartDeskShortcuts ${LNG_SPANISH} \
  "Accesos directos por la menú Inicio y el escritorio"
LangString lng_StartShortcuts ${LNG_SPANISH} \
  "Acceso directo de la menú Inicio"
LangString lng_StartShortcutsProgress ${LNG_SPANISH} \
  "Crear los accesos directos por la menú Inicio..."
LangString lng_DesktopShortcut ${LNG_SPANISH} "Acceso directo del escritorio"
LangString lng_DesktopShortcutProgress ${LNG_SPANISH} \
  "Crear acceso directo por el escritorio..."
LangString lng_RegistryProgress ${LNG_SPANISH} \
  "Crear los Llaves de Registrario..."
LangString lng_GenerateUninstaller ${LNG_SPANISH} "Generating Uninstaller..."
LangString lng_UninstallProgress ${LNG_SPANISH} \
  "Remover ${PRODUCT_NAME}.."
LangString lng_DeleteProgress ${LNG_SPANISH} "Eliminar archivos..."
LangString lng_RemoveShortcutsProgress ${LNG_SPANISH} \
  "Eliminar accesos directos..."
LangString lng_DeleteRegistryProgress ${LNG_SPANISH} \
  "Eliminar los Llaves de Registrario..."

LangString lng_CoreDesc ${LNG_SPANISH} \
  "El archivos necesita para ${PRODUCT_NAME}"
LangString lng_GTKDesc ${LNG_SPANISH} \
  "Un conjunto de herramientas GUI, utilizado por ${PRODUCT_NAME}"
LangString lng_StartDeskShortcutsDesc ${LNG_SPANISH} \
  "Crear accesos directos para el menú Inicio y/o el escritorio."
LangString lng_StartShortcutsDesc ${LNG_SPANISH} \
  "Crear un acceso directo para el menú Inicio."
LangString lng_DesktopShortcutDesc ${LNG_SPANISH} \
  "Crear un acceso directo para el escritorio."
