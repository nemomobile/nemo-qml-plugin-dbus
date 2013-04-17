TARGET = nemodbus
PLUGIN_IMPORT_PATH = org/nemomobile/dbus
QT += dbus script

TEMPLATE = lib
CONFIG += qt plugin hide_symbols
QT += declarative

target.path = $$[QT_INSTALL_IMPORTS]/$$PLUGIN_IMPORT_PATH
INSTALLS += target

qmldir.files += $$_PRO_FILE_PWD_/qmldir
qmldir.path +=  $$[QT_INSTALL_IMPORTS]/$$$$PLUGIN_IMPORT_PATH
INSTALLS += qmldir

SOURCES += \
    plugin.cpp \
    declarativedbusadaptor.cpp \
    declarativedbusinterface.cpp

HEADERS += \
    declarativedbusadaptor.h \
    declarativedbusinterface.h

MOC_DIR = $$PWD/.moc
OBJECTS_DIR = $$PWD/.obj
