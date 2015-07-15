DBusInterface
=============

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

    Whether to use the session (:cpp:member:`DBus.SessionBus`) or
    system bus (:cpp:member:`DBus.SystemBus`).
    Default: :cpp:member:`DBus.SessionBus`

.. cpp:member:: bool signalsEnabled

    When set to ``true``, signals of the D-Bus object will be connected to functions
    defined on the object (see below for examples). Default: ``false``

Signals
^^^^^^^

.. cpp:function:: void propertiesChanged()

    Emitted when properties of the D-Bus object have changed (only if the D-Bus object
    does emit signals when properties change). Right now, this does not tell which
    properties have changed and to which values.

.. versionadded:: 2.0.8

Functions
^^^^^^^^^

.. cpp:function:: void call(string method, var arguments)

    Call a D-Bus method with the name ``method`` on the object with ``arguments`` as
    either a single value or an array. For a function with no arguments, pass in ``undefined``.

.. note::

    This function supports passing basic data types and will fail if the signature of
    the remote method does not match the signature determined from the type of
    ``arguments``. The :cpp:func:`typedCall` function can be used to explicity specify the type
    of each element of ``arguments``.

.. cpp:function:: void typedCall(string method, var arguments, var callback=undefined, var errorCallback=undefined)

    Call a D-Bus method with the name ``method`` on the object with ``arguments``. Each
    parameter is described by an object:

    .. code-block:: javascript

        {
            'type': 'o'
            'value': '/org/example'
        }

    Where ``type`` is the D-Bus type the ``value`` should be marshalled as.
    ``arguments`` can be either a single object describing the parameter or an array of
    objects.

    When the function returns, call ``callback`` with a single argument that is the
    return value. The ``callback`` argument is optional, if set to ``undefined`` (the
    default), the return value will be discarded. If the function fails
    ``errorCallback`` is called if it is not set to ``undefined`` (the default).

.. cpp:function:: var getProperty(string name)

    Get the D-Bus property ``name`` from the object and return it.

.. cpp:function:: void setProperty(string name, var value)

    Set the object's D-Bus property ``name`` to ``value``.

.. versionadded:: 2.0.0

Handling D-Bus Signals
^^^^^^^^^^^^^^^^^^^^^^

If :cpp:member:`signalsEnabled` is set to ``true``, signals of the
destination object will be connected to functions on the object that have the
same name.

**Example**: Imagine a D-Bus object in service ``org.example.service`` at path
``/org/example/service`` and interface ``org.example.intf`` with two signals,
``UpdateAll`` and ``UpdateOne``. You can handle these signals this way:

.. code::

    DBusInterface {
        service: 'org.example.service'
        path: '/org/example/service'
        iface: 'org.example.intf'

        signalsEnabled: true

        function updateAll() {
            // Will be called when the "UpdateAll" signal is received
        }

        function updateOne(a, b) {
            // Will be called when the "UpdateOne" signal is received
        }
    }

.. note::

    In D-Bus, signal names usually start with an uppercase letter, but in
    QML, function names on objects must start with lowercase letters. The
    plugin connects uppercase signal names to functions where the first
    letter is lowercase (the D-Bus signal ``UpdateOne`` is handled by the
    QML/JavaScript function ``updateOne``).

Calling D-Bus Methods
^^^^^^^^^^^^^^^^^^^^^^

Remote D-Bus methods can be called using either :cpp:func:`call` or
:cpp:func:`typedCall`. :cpp:func:`call` provides a simplier calling API, only supporting
basic data types and discards any value return by the method. :cpp:func:`typedCall`
supports more data types and has callbacks for call completion and error.

**Example**: Imagine a D-Bus object in service ``org.example.service`` at path
``/org/example/service`` and interface ``org.example.intf`` with two methods:

    * ``RegisterObject`` with a single ``object path`` parameter and returning a ``bool``
    * ``Update`` with no parameters

You can call these two methods this way:

.. code::

    DBusInterface {
        service: 'org.example.service'
        path: '/org/example/service'
        iface: 'org.example.intf'

        // Local function to call remote method RegisterObject
        function registerObject(object) {
            typedCall('RegisterObject',
                      { 'type': 'o', 'value': '/example/object/path' },
                      function(result) { console.log('call completed with:', result) },
                      function() { console.log('call failed') })
        }

        // Location function to call remote method Update
        function update() {
            call('Update', undefined)
        }
    }
