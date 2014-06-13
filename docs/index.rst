Nemo Mobile D-Bus QML Plugin
============================

The **Nemo Mobile D-Bus QML Plugin** allows you to access services on the system
and session bus, as well as provide your own services. D-Bus is used for
interprocess communication. Several system services expose an interface over
D-Bus that can be used by third party software and other middleware.

* D-Bus: http://www.freedesktop.org/wiki/Software/dbus/ 
* Github: https://github.com/nemomobile/nemo-qml-plugin-dbus

What is D-Bus?
--------------

D-Bus is a message bus system, a simple way for applications to talk to one
another. In addition to interprocess communication, D-Bus helps coordinate
process lifecycle; it makes it simple and reliable to code a "single instance"
application or daemon, and to launch applications and daemons on demand when
their services are needed. (from the `D-Bus homepage`_)

.. _D-Bus homepage: http://www.freedesktop.org/wiki/Software/dbus/ 

org.nemomobile.dbus 2.0
-----------------------

To use ``DBusAdaptor`` and ``DBusInterface`` in your QML code, you need to
import the plugin using the following import statement at the top of our
QML file:

.. code-block:: javascript

    import org.nemomobile.dbus 2.0

.. note::

    The old ``import org.nemomobile.dbus 1.0`` is still available, and legacy
    code using the 0.0.x API should still work with it, but it is recommended
    that you use the 2.0 API, and update your code to work with it.

DBusAdaptor
```````````

The ``DBusAdaptor`` object can be used to provide a D-Bus service on the system or
session bus. A service can be called from other applications on the system as long
as the service is active.

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

DBusInterface
`````````````

The ``DBusInterface`` object can be used to call methods of objects on the system and
session bus, as well as receive signals (see :cpp:member:`signalsEnabled`) and read
properties of those objects.

Properties
^^^^^^^^^^

.. cpp:member:: string service

    D-Bus service name (``x.y.z``) of the destination object

.. cpp:member:: string path

    D-Bus path (``/x/y/z``) of the destination object

.. cpp:member:: string iface

    D-Bus interface name (``x.y.z``) of the destination object

.. cpp:member:: BusType bus

    Whether to use the session (:cpp:member:`DBusInterface.SessionBus`) or
    system bus (:cpp:member:`DBus.SystemBus`).
    Default: :cpp:member:`DBus.SessionBus`

.. cpp:member:: bool signalsEnabled

    When set to ``true``, signals of the D-Bus object will be available as signals
    on the object. Those signals can be connected to via the usual QML means (a
    signal with the name ``signal`` would have a ``onSignal`` handler). Default: ``false``

Signals
^^^^^^^

The ``DBusInterface`` object does not have any signals by itself. However,
if :cpp:member:`signalsEnabled` is set to ``true``, signals of the
destination object will be dynamically exposed as signals that can be
connected to.

Functions
^^^^^^^^^

.. cpp:function:: void call(string method, var arguments)

    Call a D-Bus method with the name ``method`` on the object with ``arguments``
    as argument list. For a function with no arguments, pass in ``[]`` (empty array).

.. cpp:function:: void typedCall(string method, var arguments, var callback=undefined)

    Call a D-Bus method with the name ``method`` on the object with ``arguments``
    as argument list. When the function returns, call ``callback`` with a single
    argument that is the return value. The ``callback`` argument is optional, if
    set to ``undefined`` (the default), the return value will be discarded.

.. cpp:function:: var getProperty(string name)

    Get the D-Bus property ``name`` from the object and return it.

.. cpp:function:: void setProperty(string name, var value)

    Set the object's D-Bus property ``name`` to ``value``.

.. versionadded:: 2.0.0

DBus
````

The ``DBus`` class contains enumerations used by both ``DBusAdaptor`` and
``DBusInterface``. This class cannot be instantiated, but is only used for
referring to the enumeration values.

Enumerations
^^^^^^^^^^^^

.. cpp:member:: BusType DBus.SessionBus

    D-Bus Session Bus (user session)

.. cpp:member:: BusType DBus.SystemBus

    D-Bus System Bus (system-wide)


Examples
--------

Calling a function on a session bus object
``````````````````````````````````````````

This code snippet talks to the profile daemon and sets the current profile to silent:

.. code::

    import QtQuick 2.0
    import org.nemomobile.dbus 2.0

    Item {
        DBusInterface {
            id: profiled

            service: 'com.nokia.profiled'
            iface: 'com.nokia.profiled'
            path: '/com/nokia/profiled'
        }

        Component.onCompleted: {
            // Call the "set_profile" method here
            profiled.call('set_profile', ['silent']);
        }
    }

Calling a function and getting its return value
```````````````````````````````````````````````

Because function calls are asynchronous, we need to provide a callback
to be called when the function result is available:

.. code::

    import QtQuick 2.0
    import org.nemomobile.dbus 2.0

    Item {
        DBusInterface {
            id: profiled

            service: 'com.nokia.profiled'
            iface: 'com.nokia.profiled'
            path: '/com/nokia/profiled'
        }

        Component.onCompleted: {
            // Call the "get_profile" method without arguments, and
            // when it returns, call the passed-in callback method
            profiled.typedCallWithReturn('get_profile', [], function (result) {
                // This will be called when the result is available
                console.log('Got profile: ' + result);
            });
        }
    }

Listening to signals on a D-Bus object
``````````````````````````````````````

TODO

Getting and setting properties on D-Bus objects
```````````````````````````````````````````````

TODO

Exposing a new object on the session bus
````````````````````````````````````````

TODO

API Version History
-------------------

This section lists changes in the Nemo Mobile D-Bus QML Plugin API.

Version 2.0
```````````

* Moved both ``BusType`` enums (in ``DBusInterface`` and ``DBusAdaptor``) to
  a single enum in ``DBus``
* Renamed ``destination`` property of ``DBusInterface`` to ``service``
  (to align better with the naming in ``DBusAdaptor``)
* Renamed ``busType`` property to ``bus`` for better readability
* Merged ``typedCall`` and ``typedCallWithReturn`` into a single function,
  ``typedCall`` that can handle an optional ``callback`` parameter
* Merged ``emitSignal`` and ``emitSignalWithArguments`` into a single function,
  ``emitSignal`` that can handle an optional ``arguments`` parameter
* Add new method ``setProperty`` to ``DBusInterface`` for setting D-Bus properties

Version 1.0
```````````

* Initial release (0.0.x release series), unstable API
