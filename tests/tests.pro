TEMPLATE = aux

autotest.files = auto/*.qml
autotest.path = /opt/tests/nemo-qml-plugins-qt5/dbus/auto

OTHER_FILES += \
    auto/*.qml \
    dbustestd/* \
    dbustestd/dbus/*.service

INSTALLS += autotest
