TEMPLATE = aux

autotest.files = auto/*.qml
autotest.path = /opt/tests/nemo-qml-plugins-qt5/dbus/auto

testdefinition.files = test-definition/tests.xml
testdefinition.path = /opt/tests/nemo-qml-plugins-qt5/dbus/test-definition

OTHER_FILES += \
    auto/*.qml \
    dbustestd/* \
    dbustestd/dbus/*.service \
    test-definition/*.xml \

INSTALLS += autotest testdefinition
