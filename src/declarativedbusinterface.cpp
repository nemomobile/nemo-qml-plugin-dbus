/****************************************************************************************
**
** Copyright (C) 2013 Jolla Ltd.
** Contact: Andrew den Exter <andrew.den.exter@jollamobile.com>
** All rights reserved.
**
** You may use this file under the terms of the GNU Lesser General
** Public License version 2.1 as published by the Free Software Foundation
** and appearing in the file license.lgpl included in the packaging
** of this file.
**
** This library is free software; you can redistribute it and/or
** modify it under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation
** and appearing in the file license.lgpl included in the packaging
** of this file.
** 
** This library is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
** Lesser General Public License for more details.
** 
****************************************************************************************/

#include "declarativedbusinterface.h"

#include <QMetaMethod>
#include <QDBusMetaType>
#include <QDBusMessage>
#include <QDBusConnection>
#include <QDBusObjectPath>
#include <QDBusSignature>
#include <QDBusUnixFileDescriptor>
#include <QDBusPendingCallWatcher>
#include <QDBusPendingReply>
#include <qqmlinfo.h>
#include <QJSEngine>
#include <QJSValue>
#include <QJSValueIterator>
#include <QFile>
#include <QUrl>
#include <QXmlStreamReader>

DeclarativeDBusInterface::DeclarativeDBusInterface(QObject *parent)
:   QObject(parent), m_bus(DeclarativeDBus::SessionBus), m_componentCompleted(false), m_signalsEnabled(false)
{
}

DeclarativeDBusInterface::~DeclarativeDBusInterface()
{
    foreach (QDBusPendingCallWatcher *watcher, m_pendingCalls.keys())
        delete watcher;
}

QString DeclarativeDBusInterface::service() const
{
    return m_service;
}

void DeclarativeDBusInterface::setService(const QString &service)
{
    if (m_service != service) {
        disconnectSignalHandler();

        m_service = service;
        emit serviceChanged();

        connectSignalHandler();
    }
}

QString DeclarativeDBusInterface::path() const
{
    return m_path;
}

void DeclarativeDBusInterface::setPath(const QString &path)
{
    if (m_path != path) {
        disconnectSignalHandler();

        m_path = path;
        emit pathChanged();

        connectSignalHandler();
    }
}

QString DeclarativeDBusInterface::interface() const
{
    return m_interface;
}

void DeclarativeDBusInterface::setInterface(const QString &interface)
{
    if (m_interface != interface) {
        disconnectSignalHandler();

        m_interface = interface;
        emit interfaceChanged();

        connectSignalHandler();
    }
}

DeclarativeDBus::BusType DeclarativeDBusInterface::bus() const
{
    return m_bus;
}

void DeclarativeDBusInterface::setBus(DeclarativeDBus::BusType bus)
{
    if (m_bus != bus) {
        disconnectSignalHandler();

        m_bus = bus;
        emit busChanged();

        connectSignalHandler();
    }
}

bool DeclarativeDBusInterface::signalsEnabled() const
{
    return m_signalsEnabled;
}

void DeclarativeDBusInterface::setSignalsEnabled(bool enabled)
{
    if (m_signalsEnabled != enabled) {
        if (!enabled)
            disconnectSignalHandler();

        m_signalsEnabled = enabled;
        emit signalsEnabledChanged();

        connectSignalHandler();
    }
}

QVariantList DeclarativeDBusInterface::argumentsFromScriptValue(const QJSValue &arguments)
{
    QVariantList dbusArguments;

    if (arguments.isArray()) {
        QJSValueIterator it(arguments);
        while (it.hasNext()) {
            it.next();
            // Arrays carry the size as last value
            if (!it.hasNext())
                continue;
            dbusArguments.append(it.value().toVariant());
        }
    } else if (!arguments.isUndefined()) {
        dbusArguments.append(arguments.toVariant());
    }

    return dbusArguments;
}

void DeclarativeDBusInterface::call(const QString &method, const QJSValue &arguments)
{
    QVariantList dbusArguments = argumentsFromScriptValue(arguments);

    QDBusMessage message = QDBusMessage::createMethodCall(
                m_service,
                m_path,
                m_interface,
                method);
    message.setArguments(dbusArguments);

    QDBusConnection conn = DeclarativeDBus::connection(m_bus);

    if (!conn.send(message))
        qmlInfo(this) << conn.lastError();
}

template<typename T> static QList<T> toQList(const QVariantList &lst)
{
    QList<T> arr;
    foreach(const QVariant &var, lst) {
        arr << qvariant_cast<T>(var);
    }
    return arr;
}

static QStringList toQStringList(const QVariantList &lst) {
    QStringList arr;
    foreach(const QVariant &var, lst) {
        arr << qvariant_cast<QString>(var);
    }
    return arr;
};

static QByteArray toQByteArray(const QVariantList &lst) {
    QByteArray arr;
    foreach(const QVariant &var, lst) {
        uchar tmp = static_cast<uchar>(var.toUInt());
        arr.append(static_cast<char>(tmp));
    }
    return arr;
};

static void registerDBusTypes(void)
{
    static bool done = false;

    if( !done ) {
        done = true;

        qDBusRegisterMetaType< QList<bool> >();
        qDBusRegisterMetaType< QList<int> >();
        qDBusRegisterMetaType< QList<double> >();

        qDBusRegisterMetaType< QList<quint8> >();
        qDBusRegisterMetaType< QList<quint16> >();
        qDBusRegisterMetaType< QList<quint32> >();
        qDBusRegisterMetaType< QList<quint64> >();

        qDBusRegisterMetaType< QList<qint16> >();
        qDBusRegisterMetaType< QList<qint32> >();
        qDBusRegisterMetaType< QList<qint64> >();
    }
}

static bool flattenVariantList(QVariant &var, const QVariantList &lst,
                               int typeChar)
{
    bool res = true;

    switch( typeChar ) {
    case 'b': // BOOLEAN
        var = QVariant::fromValue(toQList<bool>(lst));
        break;
    case 'y': // BYTE
        var = QVariant::fromValue(toQByteArray(lst));
        break;
    case 'q': // UINT16
        var = QVariant::fromValue(toQList<quint16>(lst));
        break;
    case 'u': // UINT32
        var = QVariant::fromValue(toQList<quint32>(lst));
        break;
    case 't': // UINT64
        var = QVariant::fromValue(toQList<quint64>(lst));
        break;
    case 'n': // INT16
        var = QVariant::fromValue(toQList<qint16>(lst));
        break;
    case 'i': // INT32
        var = QVariant::fromValue(toQList<qint32>(lst));
        break;
    case 'x': // INT64
        var = QVariant::fromValue(toQList<qint64>(lst));
        break;
    case 'd': // DOUBLE
        var = QVariant::fromValue(toQList<double>(lst));
        break;
    case 's': // STRING
        var = QVariant::fromValue(toQStringList(lst));
        break;
    default:
        res = false;
        break;
    }

    return res;
}

static bool flattenVariantArrayForceType(QVariant &var, int typeChar)
{
    return flattenVariantList(var, var.toList(), typeChar);
}

static void flattenVariantArrayGuessType(QVariant &var)
{
    /* If the value can't be converted to a variant list
     * or if the resulting list would be empty: use the
     * value without modification */
    QVariantList arr = var.toList();
    if( arr.empty() )
        return;

    /* If all items in the list do not share the same type:
     * use as is -> each value will be wrapped in variant
     * container */
    int t = arr[0].type();
    int n = arr.size();
    for( int i = 1; i < n; ++i ) {
        if( arr[i].type() != t )
            return;
    }

    switch( t ) {
    case QVariant::String: flattenVariantList(var, arr, 's'); break;
    case QVariant::Bool:   flattenVariantList(var, arr, 'b'); break;
    case QVariant::Int:    flattenVariantList(var, arr, 'i'); break;
    case QVariant::Double: flattenVariantList(var, arr, 'd'); break;
    default:
        /* Unhandled types are encoded as variant:array:variant:val
         * instead of variant:array:val what we actually want.
         */
        qWarning("unhandled array type: %d (%s)", t, QVariant::typeToName(t));
        break;
    }
}

bool
DeclarativeDBusInterface::marshallDBusArgument(QDBusMessage &msg, const QJSValue &arg)
{
    QJSValue type = arg.property(QLatin1String("type"));

    if (!type.isString()) {
        qWarning() << "DeclarativeDBusInterface::typedCall - Invalid type";
        qmlInfo(this) << "DeclarativeDBusInterface::typedCall - Invalid type";
        return false;
    }

    QJSValue value = arg.property(QLatin1String("value"));

    if( value.isNull() || value.isUndefined() ) {
        qWarning() << "DeclarativeDBusInterface::typedCall - Invalid argument";
        qmlInfo(this) << "DeclarativeDBusInterface::typedCall - Invalid argument";
        return false;
    }

    QString t = type.toString();
    if (t.length() == 1) {
        switch (t.at(0).toLatin1()) {
        case 'y': // BYTE
            msg << QVariant::fromValue(static_cast<quint8>(value.toUInt()));
            return true;

        case 'q': // UINT16
            msg << QVariant::fromValue(static_cast<quint16>(value.toUInt()));
            return true;

        case 'u': // UINT32
            msg << QVariant::fromValue(static_cast<quint32>(value.toUInt()));
            return true;

        case 't': // UINT64
            msg << QVariant::fromValue(static_cast<quint64>(value.toVariant().toULongLong()));
            return true;

        case 'n': // INT16
            msg << QVariant::fromValue(static_cast<qint16>(value.toInt()));
            return true;

        case 'i': // INT32
            msg << QVariant::fromValue(static_cast<qint32>(value.toInt()));
            return true;

        case 'x': // INT64
            msg << QVariant::fromValue(static_cast<qint64>(value.toVariant().toLongLong()));
            return true;

        case 'b': // BOOLEAN
            msg << value.toBool();
            return true;

        case 'd': // DOUBLE
            msg << value.toNumber();
            return true;

        case 's': // STRING
            msg << value.toString();
            return true;

        case 'o': // OBJECT_PATH
            msg << QVariant::fromValue(QDBusObjectPath(value.toString()));
            return true;

        case 'g': // SIGNATURE
            msg << QVariant::fromValue(QDBusSignature(value.toString()));
            return true;

        case 'h': // UNIX_FD
            msg << QVariant::fromValue(QDBusUnixFileDescriptor(value.toInt()));
            return true;

        case 'v': // VARIANT
            {
                QVariant var = value.toVariant();
                flattenVariantArrayGuessType(var);
                msg << QVariant::fromValue(QDBusVariant(var));
            }
            return true;

        default:
            break;
        }
    } else if (t.length() == 2 && (t.at(0).toLatin1() == 'a')) {
        // The result must be an array of typed data
        if (!value.isArray()) {
            qWarning() << "Invalid value for type specifier:" << t << "v:" << value.toVariant();
            qmlInfo(this) << "Invalid value for type specifier:" << t << "v:" << value.toVariant();
            return false;
        }

        QVariant vec = value.toVariant();
        int type = t.at(1).toLatin1();

        if( flattenVariantArrayForceType(vec, type) ) {
            msg << vec;
            return true;
        }
    }

    qWarning() << "DeclarativeDBusInterface::typedCall - Invalid type specifier:" << t;
    qmlInfo(this) << "DeclarativeDBusInterface::typedCall - Invalid type specifier:" << t;
    return false;
}

QDBusMessage
DeclarativeDBusInterface::constructMessage(const QString &service,
                                           const QString &path,
                                           const QString &interface,
                                           const QString &method,
                                           const QJSValue &arguments)
{
    registerDBusTypes();

    QDBusMessage message = QDBusMessage::createMethodCall(service, path, interface, method);

    if (arguments.isArray()) {
        quint32 len = arguments.property(QLatin1String("length")).toUInt();
        for (quint32 i = 0; i < len; ++i) {
            if( !marshallDBusArgument(message, arguments.property(i)) )
                return QDBusMessage();
        }
    } else if (!arguments.isUndefined()) {
        // arguments is a singular typed value
        if( !marshallDBusArgument(message, arguments) )
            return QDBusMessage();
    }
    return message;
}

QVariant DeclarativeDBusInterface::parse(const QDBusArgument &argument)
{
    switch (argument.currentType()) {
    case QDBusArgument::BasicType: {
        QVariant v = argument.asVariant();
        if (v.userType() == qMetaTypeId<QDBusObjectPath>())
            return v.value<QDBusObjectPath>().path();
        else if (v.userType() == qMetaTypeId<QDBusSignature>())
            return v.value<QDBusSignature>().signature();
        else
            return v;
    }
    case QDBusArgument::VariantType: {
        QVariant v = argument.asVariant().value<QDBusVariant>().variant();
        if (v.userType() == qMetaTypeId<QDBusArgument>())
            return parse(v.value<QDBusArgument>());
        else
            return v;
    }
    case QDBusArgument::ArrayType: {
        QVariantList list;
        argument.beginArray();
        while (!argument.atEnd())
            list.append(parse(argument));
        argument.endArray();
        return list;
    }
    case QDBusArgument::StructureType: {
        QVariantList list;
        argument.beginStructure();
        while (!argument.atEnd())
            list.append(parse(argument));
        argument.endStructure();
        return QVariant::fromValue(list);
    }
    case QDBusArgument::MapType: {
        QVariantMap map;
        argument.beginMap();
        while (!argument.atEnd()) {
            argument.beginMapEntry();
            QVariant key = parse(argument);
            QVariant value = parse(argument);
            map.insert(key.toString(), value);
            argument.endMapEntry();
        }
        argument.endMap();
        return map;
    }
    default:
        return QVariant();
        break;
    }
}

bool DeclarativeDBusInterface::typedCall(const QString &method, const QJSValue &arguments, const QJSValue &callback,
                                         const QJSValue &errorCallback)
{
    QDBusMessage message = constructMessage(m_service, m_path, m_interface, method, arguments);
    if (message.type() == QDBusMessage::InvalidMessage) {
        qmlInfo(this) << "Invalid message, cannot call method:" << method;
        return false;
    }

    QDBusConnection conn = DeclarativeDBus::connection(m_bus);

    if (callback.isUndefined()) {
        // Call without waiting for return value (callback is undefined)
        if (!conn.send(message)) {
            qmlInfo(this) << conn.lastError();
        }
        return true;
    }

    // If we have a non-undefined callback, it must be callable
    if (!callback.isCallable()) {
        qmlInfo(this) << "Callback argument is not a function";
        return false;
    }

    if (!errorCallback.isUndefined() && !errorCallback.isCallable()) {
        qmlInfo(this) << "Error callback argument is not a function or undefined";
        return false;
    }

    QDBusPendingCall pendingCall = conn.asyncCall(message);
    QDBusPendingCallWatcher *watcher = new QDBusPendingCallWatcher(pendingCall);
    connect(watcher, SIGNAL(finished(QDBusPendingCallWatcher*)),
            this, SLOT(pendingCallFinished(QDBusPendingCallWatcher*)));
    m_pendingCalls.insert(watcher, qMakePair(callback, errorCallback));
    return true;
}

QVariant DeclarativeDBusInterface::getProperty(const QString &name)
{
    QDBusMessage message =
        QDBusMessage::createMethodCall(m_service, m_path,
                                       QLatin1String("org.freedesktop.DBus.Properties"),
                                       QLatin1String("Get"));

    QVariantList args;
    args.append(m_interface);
    args.append(name);

    message.setArguments(args);

    QDBusConnection conn = DeclarativeDBus::connection(m_bus);

    QDBusMessage reply = conn.call(message);
    if (reply.type() == QDBusMessage::ErrorMessage)
        return QVariant();
    if (reply.arguments().isEmpty())
        return QVariant();

    return unwind(reply.arguments().first());
}

void DeclarativeDBusInterface::setProperty(const QString &name, const QVariant &value)
{
    QDBusMessage message = QDBusMessage::createMethodCall(m_service, m_path,
            QLatin1String("org.freedesktop.DBus.Properties"),
            QLatin1String("Set"));

    QVariantList args;
    args.append(m_interface);
    args.append(name);
    args.append(value);

    QDBusConnection conn = DeclarativeDBus::connection(m_bus);
    if (!conn.send(message))
        qmlInfo(this) << conn.lastError();
}

void DeclarativeDBusInterface::classBegin()
{
}

void DeclarativeDBusInterface::componentComplete()
{
    m_componentCompleted = true;
    connectSignalHandler();
}

QVariant DeclarativeDBusInterface::unwind(const QVariant &val, int depth)
{
    /* Limit recursion depth to protect against type conversions
     * that fail to converge to basic qt types within qt variant.
     *
     * Using limit >= DBUS_MAXIMUM_TYPE_RECURSION_DEPTH (=32) should
     * mean we do not bail out too soon on deeply nested but othewise
     * valid dbus messages. */
    static const int maximum_dept = 32;

    /* Default to QVariant with isInvalid() == true */
    QVariant res;

    const int type = val.userType();

    if( ++depth > maximum_dept ) {
        /* Leave result to invalid variant */
        qWarning() << "Too deep recursion detected at userType:" << type;
        qmlInfo(this) << "Too deep recursion detected at userType:" << type;
    }
    else if (type == QVariant::ByteArray ) {
        /* Is built-in type, but does not get correctly converted
         * to qml domain -> convert to variant list */
        QByteArray arr = val.toByteArray();
        QVariantList lst;
        for( int i = 0; i < arr.size(); ++i )
            lst <<QVariant::fromValue(static_cast<quint8>(arr[i]));
        res = QVariant::fromValue(lst);
    }
    else if (type == val.type()) {
        /* Already is built-in qt type, use as is */
        res = val;
    } else if (type == qMetaTypeId<QDBusVariant>()) {
        /* Convert QDBusVariant to QVariant */
        res = unwind(val.value<QDBusVariant>().variant(), depth);
    } else if (type == qMetaTypeId<QDBusObjectPath>()) {
        /* Convert QDBusObjectPath to QString */
        res = val.value<QDBusObjectPath>().path();
    } else if (type == qMetaTypeId<QDBusSignature>()) {
        /* Convert QDBusSignature to QString */
        res =  val.value<QDBusSignature>().signature();
    } else if (type == qMetaTypeId<QDBusUnixFileDescriptor>()) {
        /* Convert QDBusUnixFileDescriptor to int */
        res =  val.value<QDBusUnixFileDescriptor>().fileDescriptor();
    } else if (type == qMetaTypeId<QDBusArgument>()) {
        /* Try to deal with everything QDBusArgument could be ... */
        const QDBusArgument &arg = val.value<QDBusArgument>();
        const QDBusArgument::ElementType elem = arg.currentType();
        switch (elem) {
        case QDBusArgument::BasicType:
            /* Most of the basic types should be convertible to QVariant.
             * Recurse anyway to deal with object paths and the like. */
            res = unwind(arg.asVariant(), depth);
            break;

        case QDBusArgument::VariantType:
            /* Try to convert to QVariant. Recurse to check content */
            res = unwind(arg.asVariant().value<QDBusVariant>().variant(),
                         depth);
            break;

        case QDBusArgument::ArrayType:
            /* Convert dbus array to QVariantList */
            {
                QVariantList list;
                arg.beginArray();
                while (!arg.atEnd()) {
                    QVariant tmp = arg.asVariant();
                    list.append(unwind(tmp, depth));
                }
                arg.endArray();
                res = list;
            }
            break;

        case QDBusArgument::StructureType:
            /* Convert dbus struct to QVariantList */
            {
                QVariantList list;
                arg.beginStructure();
                while (!arg.atEnd()) {
                    QVariant tmp = arg.asVariant();
                    list.append(unwind(tmp, depth));
                }
                arg.endStructure();
                res = QVariant::fromValue(list);
            }
            break;

        case QDBusArgument::MapType:
            /* Convert dbus dict to QVariantMap */
            {
                QVariantMap map;
                arg.beginMap();
                while (!arg.atEnd()) {
                    arg.beginMapEntry();
                    QVariant key = arg.asVariant();
                    QVariant val = arg.asVariant();
                    map.insert(unwind(key, depth).toString(),
                               unwind(val, depth));
                    arg.endMapEntry();
                }
                arg.endMap();
                res = map;
            }
            break;

        default:
            /* Unhandled types produce invalid QVariant */
            qWarning() << "Unhandled QDBusArgument element type:" << elem;
            qmlInfo(this) << "Unhandled QDBusArgument element type:" << elem;
            break;
        }
    } else {
        /* Default to using as is. This should leave for example QDBusError
         * types in a form that does not look like a string to qml code. */
        res = val;
        qWarning() << "Unhandled QVariant userType:" << type;
        qmlInfo(this) << "Unhandled QVariant userType:" << type;
    }

    return res;
}

void DeclarativeDBusInterface::pendingCallFinished(QDBusPendingCallWatcher *watcher)
{
    QPair<QJSValue, QJSValue> callbacks = m_pendingCalls.take(watcher);

    watcher->deleteLater();

    QDBusPendingReply<> reply = *watcher;

    if (reply.isError()) {
        qmlInfo(this) << reply.error();
        QJSValue errorCallback = callbacks.second;
        if (errorCallback.isCallable()) {
            QJSValue result = errorCallback.call();
            if (result.isError()) {
                qmlInfo(this) << "Error executing error handling callback";
            }
        }
        return;
    }

    QJSValue callback = callbacks.first;
    if (!callback.isCallable())
        return;

    QDBusMessage message = reply.reply();

    QJSValueList callbackArguments;

    QVariantList arguments = message.arguments();
    foreach (QVariant argument, arguments) {
        callbackArguments << callback.engine()->toScriptValue<QVariant>(unwind(argument));
    }

    QJSValue result = callback.call(callbackArguments);
    if (result.isError()) {
        qmlInfo(this) << "Error executing callback";
    }
}

void DeclarativeDBusInterface::signalHandler(const QDBusMessage &message)
{
    QVariantList arguments = message.arguments();
    QVariantList normalized;

    QGenericArgument args[10];

    for (int i = 0; i < qMin(arguments.length(), 10); ++i) {
        const QVariant &tmp = arguments.at(i);
        normalized.append(unwind(tmp));
        const QVariant &arg = normalized.last();
        args[i] = Q_ARG(QVariant, arg);
    }

    QMetaMethod method = m_signals.value(message.member());
    if (!method.isValid())
        return;

    method.invoke(this, args[0], args[1], args[2], args[3], args[4], args[5], args[6], args[7],
                  args[8], args[9]);
}

void DeclarativeDBusInterface::connectSignalHandlerCallback(const QString &introspectionData)
{
    QStringList dbusSignals;

    QXmlStreamReader xml(introspectionData);
    while (!xml.atEnd()) {
        if (!xml.readNextStartElement())
            continue;

        if (xml.name() == QLatin1String("node"))
            continue;

        if (xml.name() != QLatin1String("interface") ||
            xml.attributes().value(QLatin1String("name")) != m_interface) {
            xml.skipCurrentElement();
            continue;
        }

        while (!xml.atEnd()) {
            if (!xml.readNextStartElement())
                break;

            if (xml.name() == QLatin1String("signal"))
                dbusSignals.append(xml.attributes().value(QLatin1String("name")).toString());

            xml.skipCurrentElement();
        }
    }

    if (dbusSignals.isEmpty())
        return;

    QDBusConnection conn = DeclarativeDBus::connection(m_bus);

    // Skip over signals defined in DeclarativeDBusInterface and its parent classes
    // so only signals defined in qml are connected to.
    for (int i = staticMetaObject.methodCount(); i < metaObject()->methodCount(); ++i) {
        QMetaMethod method = metaObject()->method(i);

        QString methodName = method.name();

        // API version 1.0 name mangling:
        // Connect QML signals with the prefix 'rc' followed by an upper-case
        // letter to DBus signals of the same name minus the prefix.
        if (methodName.length() > 2
                && methodName.startsWith(QStringLiteral("rc"))
                && methodName.at(2).isUpper()) {
            methodName.remove(0, 2);
        } else if (methodName.length() >= 2) {
            // API version 2.0 name mangling:
            //  "methodName" -> "MethodName" (if a corresponding signal exists)
            QString methodNameUpperCase = methodName.at(0).toUpper() +
                methodName.mid(1);

            // Only do name mangling if the lower case version does not exist,
            // and the upper case version does exist.
            if (!dbusSignals.contains(methodName) &&
                 dbusSignals.contains(methodNameUpperCase)) {
                methodName = methodNameUpperCase;
            }
        }

        if (!dbusSignals.contains(methodName))
            continue;

        dbusSignals.removeOne(methodName);

        m_signals.insert(methodName, method);
        conn.connect(m_service, m_path, m_interface, methodName,
                     this, SLOT(signalHandler(QDBusMessage)));

        if (dbusSignals.isEmpty())
            break;
    }
}

void DeclarativeDBusInterface::disconnectSignalHandler()
{
    if (m_signals.isEmpty())
        return;

    QDBusConnection conn = DeclarativeDBus::connection(m_bus);

    foreach (const QString &signal, m_signals.keys()) {
        conn.disconnect(m_service, m_path, m_interface, signal,
                        this, SLOT(signalHandler(QDBusMessage)));
    }

    m_signals.clear();
}

void DeclarativeDBusInterface::connectSignalHandler()
{
    if (!m_componentCompleted || !m_signalsEnabled)
        return;

    QDBusMessage message =
        QDBusMessage::createMethodCall(m_service, m_path,
                                       QLatin1String("org.freedesktop.DBus.Introspectable"),
                                       QLatin1String("Introspect"));

    if (message.type() == QDBusMessage::InvalidMessage)
        return;

    QDBusConnection conn = DeclarativeDBus::connection(m_bus);

    if (!conn.callWithCallback(message, this, SLOT(connectSignalHandlerCallback(QString))))
        qmlInfo(this) << "Failed to introspect interface" << conn.lastError();
}
