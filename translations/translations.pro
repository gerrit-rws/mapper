#
#    Copyright 2016 Kai Pastor
#    
#    This file is part of OpenOrienteering.
# 
#    OpenOrienteering is free software: you can redistribute it and/or modify
#    it under the terms of the GNU General Public License as published by
#    the Free Software Foundation, either version 3 of the License, or
#    (at your option) any later version.
# 
#    OpenOrienteering is distributed in the hope that it will be useful,
#    but WITHOUT ANY WARRANTY; without even the implied warranty of
#    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#    GNU General Public License for more details.
# 
#    You should have received a copy of the GNU General Public License
#    along with OpenOrienteering.  If not, see <http://www.gnu.org/licenses/>.

TEMPLATE = aux
CONFIG  -= debug_and_release
INSTALLS = translations

qtPrepareTool(LRELEASE, lrelease)
qtPrepareTool(LCONVERT, lconvert)

exists($$LRELEASE):exists($$LCONVERT) {
    translations.CONFIG += no_check_exist # undocumented qmake
    android: translations.path = /assets/translations
    win32:   translations.path = /translations
    
    LANGUAGES = cs de en es et fi fr hu it ja lv nb nl pl pt_BR ru sv uk
    QT_TRANSLATIONS_PREFIXES = qtbase
    for(lang, LANGUAGES) {
        TRANSLATIONS += OpenOrienteering_$${lang}.ts

        qm_file = OpenOrienteering_$${lang}.qm
        oo_$${lang}.target    = $$qm_file
        oo_$${lang}.depends   = $$PWD/OpenOrienteering_$${lang}.ts
        oo_$${lang}.commands  = $$LRELEASE -qm $$qm_file $$PWD/OpenOrienteering_$${lang}.ts
        QMAKE_EXTRA_TARGETS  += oo_$${lang}
        QMAKE_CLEAN          += $$qm_file
        translations.depends += $$qm_file
        translations.files   += $$OUT_PWD/$$qm_file
        
        qt_qm_files =
        for(qt_prefix, QT_TRANSLATIONS_PREFIXES) {
            qt_qm_file = $$[QT_INSTALL_TRANSLATIONS]/$${qt_prefix}_$${lang}.qm
            exists($$qt_qm_file): qt_qm_files += $$qt_qm_file
        }
        !isEmpty(qt_qm_files) {
            qm_file = qt_$${lang}.qm
            qt_$${lang}.target    = $$qm_file
            qt_$${lang}.depends   = $$qt_qm_files
            qt_$${lang}.commands  = $$LCONVERT -o $$qm_file $$qt_qm_files
            QMAKE_EXTRA_TARGETS  += qt_$${lang}
            QMAKE_CLEAN          += $$qm_file
            translations.depends += $$qm_file
            translations.files   += $$OUT_PWD/$$qm_file
        } else:exists($$PWD/qt_$${lang}.ts) {
            qm_file = qt_$${lang}.qm
            qt_$${lang}.target    = $$qm_file
            qt_$${lang}.depends   = $$PWD/qt_$${lang}.ts
            qt_$${lang}.commands  = $$LRELEASE -qm $$qm_file $$PWD/qt_$${lang}.ts
            QMAKE_EXTRA_TARGETS  += qt_$${lang}
            QMAKE_CLEAN          += $$qm_file
            translations.depends += $$qm_file
            translations.files   += $$OUT_PWD/$$qm_file
        }
    }
}
