Nemo Mobile D-Bus QML Plugin
============================

.. warning::

   The API is not yet finalized and subject to change before the 1.0.0 release.

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

org.nemomobile.dbus 1.0
-----------------------

To use ``DBusAdaptor`` and ``DBusInterface`` in your QML code, you need to
import the plugin using the following import statement at the top of our
QML file:

.. code-block:: javascript

    import org.nemomobile.dbus 1.0

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

.. cpp:member:: BusType busType

    Whether to use the session (:cpp:member:`DBusAdaptor.SessionBus`) or
    system bus (:cpp:member:`DBusAdaptor.SystemBus`).
    Default: :cpp:member:`DBusAdaptor.SessionBus`

Enumerations
^^^^^^^^^^^^

.. note::

    This should be merged into a ``DBus.BusType`` enum that can be used by both
    ``DBusInterface`` and ``DBusAdaptor``.

.. cpp:member:: BusType DBusAdaptor.SessionBus

    D-Bus Session Bus (user session)

.. cpp:member:: BusType DBusAdaptor.SystemBus

    D-Bus System Bus (system-wide)


Signals
^^^^^^^

None.

Functions
^^^^^^^^^

.. cpp:function:: void emitSignal(string name)

    Emit a signal with the given ``name`` and no arguments.

.. cpp:function:: void emitSignalWithArguments(string name, var arguments)

    Emit a signal with the given ``name`` and ``arguments``.

.. note::

    Maybe we can combine both ``emitSignal`` and ``emitSignalWithArguments``
    as just a single function ``emitSignal`` that takes an optional second parameter?

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

.. cpp:member:: BusType busType

    Whether to use the session (:cpp:member:`DBusInterface.SessionBus`) or
    system bus (:cpp:member:`DBusInterface.SystemBus`).
    Default: :cpp:member:`DBusInterface.SessionBus`

.. cpp:member:: bool signalsEnabled

    When set to ``true``, signals of the D-Bus object will be available as signals
    on the object. Those signals can be connected to via the usual QML means (a
    signal with the name ``signal`` would have a ``onSignal`` handler). Default: ``false``

Enumerations
^^^^^^^^^^^^

.. note::

    This should be merged into a ``DBus.BusType`` enum that can be used by both
    ``DBusInterface`` and ``DBusAdaptor``.

.. cpp:member:: BusType DBusInterface.SessionBus

    D-Bus Session Bus (user session)

.. cpp:member:: BusType DBusInterface.SystemBus

    D-Bus System Bus (system-wide)

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

.. cpp:function:: void typedCall(string method, var arguments)

    TODO

.. note::

    Same? Why is that needed? What's the difference? When would I use this
    in favor of ``call``? Maybe we only need one of both?

.. cpp:function:: void typedCallWithReturn(string method, var arguments, var callback)

    Call a D-Bus method with the name ``method`` on the object with ``arguments``
    as argument list. When the function returns, call ``callback`` with a single
    argument that is the return value.

.. note::

    We might actually want to have just one ``call`` function that takes the name,
    the arguments and an optional callback - if the callback is not set, it will
    act like ``typedCall``, otherwise it will act like ``typedCallWithReturn``?

.. cpp:function:: var getProperty(string name)

    Get the D-Bus property ``name`` from the object and return it.

.. note::

    Is it also possible ot set properties on the D-Bus object? If so, how?

Examples
--------

Calling a function on a session bus object
``````````````````````````````````````````

This code snippet talks to the profile daemon and sets the current profile to silent:

.. code::

    import QtQuick 2.0
    import org.nemomobile.dbus 1.0

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
    import org.nemomobile.dbus 1.0

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
