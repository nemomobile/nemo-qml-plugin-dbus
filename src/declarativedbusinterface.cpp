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
#include <QDBusMessage>
#include <QDBusConnection>
#include <QDBusObjectPath>
#include <QDBusSignature>
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

namespace {

template<typename T>
T extractTypedList(const QVariantList &list)
{
    T rv;
    foreach (const QVariant &element, list) {
        rv.append(element.value<typename T::value_type>());
    }
    return rv;
}

QVariant marshallDBusArgument(const QJSValue &arg)
{
    QJSValue type = arg.property(QLatin1String("type"));
    QJSValue value = arg.property(QLatin1String("value"));

    if (!type.isString()) {
        qWarning() << "DeclarativeDBusInterface::typedCall - Invalid type";
        return QVariant();
    }

    bool valueValid = !value.isNull() && !value.isUndefined();

    if (valueValid) {
        QString t = type.toString();
        if (t.length() == 1) {
            switch (t.at(0).toLatin1()) {
                case 'y': return QVariant(static_cast<signed char>((value.toInt() & 0x7f) | (value.toInt() < 0 ? 0x80 : 0)));
                case 'n': return QVariant(static_cast<qint16>((value.toInt() & 0x7fff) | (value.toInt() < 0 ? 0x8000 : 0)));
                case 'q': return QVariant(static_cast<quint16>(value.toUInt()));
                case 'i': return QVariant(value.toInt());
                case 'h':
                case 'u': return QVariant(value.toUInt());
                case 'x': return QVariant(static_cast<qint64>(value.toInt()));
                case 't': return QVariant(static_cast<quint64>(value.toUInt()));
                case 'v': return value.toVariant();
                case 'b': return QVariant(value.toBool());
                case 'd': return QVariant(static_cast<double>(value.toNumber()));
                case 's': return QVariant(value.toString());
                case 'o': return QVariant::fromValue(QDBusObjectPath(value.toString()));
                case 'g': return QVariant::fromValue(QDBusSignature(value.toString()));
                default: break;
            }
        } else if (t.length() == 2 && (t.at(0).toLatin1() == 'a')) {
            // The result must be an array of typed data
            if (!value.isArray()) {
                qWarning() << "Invalid value for type specifier:" << t << "v:" << value.toVariant();
            }  else {
                const QVariantList list = value.toVariant().toList();
                switch (t.at(1).toLatin1()) {
                    case 's': return QVariant(extractTypedList<QStringList>(list));
                    default: break;
                }
            }
        }
        qWarning() << "DeclarativeDBusInterface::typedCall - Invalid type specifier:" << t;
    } else {
        qWarning() << "DeclarativeDBusInterface::typedCall - Invalid argument";
    }

    return QVariant();
}

QDBusMessage constructMessage(const QString &service, const QString &path,
                              const QString &interface, const QString &method,
                              const QJSValue &arguments)
{
    QVariantList dbusArguments;

    if (arguments.isArray()) {
        quint32 len = arguments.property(QLatin1String("length")).toUInt();
        for (quint32 i = 0; i < len; ++i) {
            QVariant value = marshallDBusArgument(arguments.property(i));
            if (!value.isValid()) {
                return QDBusMessage();
            }
            dbusArguments.append(value);
        }
    } else if (!arguments.isUndefined()) {
        // arguments is a singular typed value
        QVariant value = marshallDBusArgument(arguments);
        if (!value.isValid()) {
            return QDBusMessage();
        }
        dbusArguments.append(value);
    }

    QDBusMessage message = QDBusMessage::createMethodCall(service, path, interface, method);
    message.setArguments(dbusArguments);

    return message;
}

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

    QVariant v = reply.arguments().first();
    if (v.userType() == qMetaTypeId<QDBusVariant>()) {
        QVariant arg = v.value<QDBusVariant>().variant();
        if (arg.userType() == qMetaTypeId<QDBusArgument>())
            return parse(arg.value<QDBusArgument>());
        else
            return arg;
    } else {
        return v;
    }
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
    message.setArguments(args);

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
        if (argument.userType() == qMetaTypeId<QDBusArgument>())
            argument = parse(argument.value<QDBusArgument>());
        else if (argument.userType() == qMetaTypeId<QDBusObjectPath>())
            argument = argument.value<QDBusObjectPath>().path();
        callbackArguments << callback.engine()->toScriptValue<QVariant>(argument);
    }

    QJSValue result = callback.call(callbackArguments);
    if (result.isError()) {
        qmlInfo(this) << "Error executing callback";
    }
}

void DeclarativeDBusInterface::signalHandler(const QDBusMessage &message)
{
    QVariantList arguments = message.arguments();

    QGenericArgument args[10];

    for (int i = 0; i < qMin(arguments.length(), 10); ++i) {
        const QVariant &arg = arguments.at(i);
        args[i] = QGenericArgument(arg.typeName(), arg.data());
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
