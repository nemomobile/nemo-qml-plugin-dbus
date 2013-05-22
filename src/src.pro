TARGET = nemodbus
PLUGIN_IMPORT_PATH = org/nemomobile/dbus
equals(QT_MAJOR_VERSION, 4): QT += dbus script declarative
equals(QT_MAJOR_VERSION, 5): QT += dbus qml

QT -= gui

TEMPLATE = lib
CONFIG += qt plugin hide_symbols

equals(QT_MAJOR_VERSION, 5): DEFINES += QT_VERSION_5

equals(QT_MAJOR_VERSION, 4): target.path = $$[QT_INSTALL_IMPORTS]/$$PLUGIN_IMPORT_PATH
equals(QT_MAJOR_VERSION, 5): target.path = $$[QT_INSTALL_QML]/$$PLUGIN_IMPORT_PATH
INSTALLS += target

qmldir.files += $$_PRO_FILE_PWD_/qmldir
qmldir.path +=  $$target.path
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
