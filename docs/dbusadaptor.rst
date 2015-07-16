DBusAdaptor
===========

The ``DBusAdaptor`` object can be used to provide a D-Bus service on the system or
session bus. A service can be called from other applications on the system as long
as the service is active.

``DBusAdaptor`` is intended to provide a means of exposing simple objects over D-Bus.
Property values and method arguments are automatically converted between QML/JS and
D-Bus. There is limited control over this process. For more complex use cases it is
recommended to use C++ and the Qt DBus module.

Properties
^^^^^^^^^^

.. cpp:member:: string service

    D-Bus service name (``x.y.z``) for this object

.. cpp:member:: string path

    D-Bus path (``/x/y/z``) where this object will be exposed

.. cpp:member:: string iface

    D-Bus interface name (``x.y.z``) that this object supports

.. cpp:member:: string xml

    XML string containing the D-Bus introspection metadata for
    this object

.. cpp:member:: BusType bus

    Whether to use the session (:cpp:member:`DBus.SessionBus`) or
    system bus (:cpp:member:`DBus.SystemBus`).
    Default: :cpp:member:`DBus.SessionBus`


Signals
^^^^^^^

None.

Functions
^^^^^^^^^

.. cpp:function:: void emitSignal(string name, var arguments=undefined)

    Emit a signal with the given ``name`` and ``arguments``. If ``arguments`` is
    undefined (the default), then the signal will be emitted without arguments.

Exposing an object on D-Bus
^^^^^^^^^^^^^^^^^^^^^^^^^^^

The following code demonstrates how to expose an object on the session bus. The
``com.example.service`` service name will be registered and an object at
``/com/example/service`` will be registered supporting the ``com.example.service``
interface in addition to the common interfaces ``org.freedesktop.DBus.Properties``,
``org.freedesktop.DBus.Introspectable`` and ``org.freedesktop.DBus.Peer``.

All properties and methods of the ``DBusAdaptor`` will be accessible via D-Bus. Only those
properties and methods declared in the :cpp:member:`xml` will be discoverable via D-Bus
introspection.

.. code::

    import QtQuick 2.0
    import org.nemomobile.dbus 2.0

    Item {
        DBusAdaptor {
            id: dbus

            property bool needUpdate: true

            service: 'com.example.service'
            iface: 'com.example.service'
            path: '/com/example/service'

            xml: '  <interface name="com.example.service">\n' +
                 '    <method name="update" />\n' +
                 '    <property name="needUpdate" type="b" access="readwrite" />\n' +
                 '  </interface>\n'

            function update() {
                console.log("Update called")
            }
        }
    }
