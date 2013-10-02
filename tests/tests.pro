TEMPLATE = subdirs

equals(QT_MAJOR_VERSION, 5) {
    target.files = auto-qt5/*
    target.path = /opt/tests/nemo-qml-plugins/dbus-qt5/auto
}

INSTALLS += target
