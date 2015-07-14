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

This QML plugin provides the following types:

.. toctree::
    :maxdepth: 1
    :titlesonly:

    dbus
    dbusadaptor
    dbusinterface

.. note::

    The old ``import org.nemomobile.dbus 1.0`` is still available, and legacy
    code using the 0.0.x API should still work with it, but it is recommended
    that you use the 2.0 API, and update your code to work with it.

Examples
--------

There are a number of examples which demonstrate how to use various aspects of the API.

.. toctree::
    :maxdepth: 2

    examples


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
* The connection handling when ``signalsEnabled`` is ``true`` can now automatically
  connect lowercase JavaScript handler functions to uppercase D-Bus signals if a
  corresponding lowercase D-Bus signal does not exist on the object.

Version 1.0
```````````

* Initial release (0.0.x release series), unstable API
