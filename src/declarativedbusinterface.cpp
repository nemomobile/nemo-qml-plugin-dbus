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

#include <QDBusMessage>
#include <QDBusConnection>
#include <QDBusObjectPath>
#include <QDBusSignature>
#include <QDBusPendingCallWatcher>
#include <QDBusPendingReply>
#ifdef QT_VERSION_5
# include <qqmlinfo.h>
# include <QJSEngine>
# include <QJSValue>
# include <QJSValueIterator>
#else
# include <QScriptEngine>
# include <QScriptValue>
# include <qdeclarativeinfo.h>
#endif
#include <QFile>
#include <QUrl>

QT_BEGIN_NAMESPACE
#if QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)
#define QScriptValueList QJSValueList
#endif
QT_END_NAMESPACE

DeclarativeDBusInterface::DeclarativeDBusInterface(QObject *parent)
    : QObject(parent), m_busType(SessionBus)
{
}

DeclarativeDBusInterface::~DeclarativeDBusInterface()
{
    foreach (QDBusPendingCallWatcher *watcher, m_pendingCalls.keys())
        delete watcher;
}

QString DeclarativeDBusInterface::destination() const
{
    return m_destination;
}

void DeclarativeDBusInterface::setDestination(const QString &destination)
{
    if (m_destination != destination) {
        m_destination = destination;
        emit destinationChanged();
    }
}

QString DeclarativeDBusInterface::path() const
{
    return m_path;
}

void DeclarativeDBusInterface::setPath(const QString &path)
{
    if (m_path != path) {
        m_path = path;
        emit pathChanged();
    }
}

QString DeclarativeDBusInterface::interface() const
{
    return m_interface;
}

void DeclarativeDBusInterface::setInterface(const QString &interface)
{
    if (m_interface != interface) {
        m_interface = interface;
        emit interfaceChanged();
    }
}

DeclarativeDBusInterface::BusType DeclarativeDBusInterface::busType() const
{
    return m_busType;
}

void DeclarativeDBusInterface::setBusType(DeclarativeDBusInterface::BusType busType)
{
    if (m_busType != busType) {
        m_busType = busType;
        emit busTypeChanged();
    }
}

void DeclarativeDBusInterface::call(const QString &method, const QScriptValue &arguments)
{
    QVariantList dbusArguments;

    if (arguments.isArray()) {
#ifdef QT_VERSION_5
        QJSValueIterator it(arguments);
        while (it.hasNext()) {
            it.next();
            // Arrays carry the size as last value
            if (!it.hasNext())
                continue;
            dbusArguments.append(it.value().toVariant());
        }
#else
        qScriptValueToSequence(arguments, dbusArguments);
#endif
    } else if (!arguments.isUndefined()) {
#ifdef QT_VERSION_5
        dbusArguments.append(arguments.toVariant());
#else
        dbusArguments.append(qscriptvalue_cast<QVariant>(arguments));
#endif
    }

    QDBusMessage message = QDBusMessage::createMethodCall(
                m_destination,
                m_path,
                m_interface,
                method);
    message.setArguments(dbusArguments);

    QDBusConnection conn = m_busType == SessionBus ? QDBusConnection::sessionBus()
                                                   : QDBusConnection::systemBus();

    if (!conn.send(message))
        qmlInfo(this) << conn.lastError();
}

namespace {

QVariant marshallDBusArgument(const QScriptValue &arg)
{
    QScriptValue type = arg.property(QLatin1String("type"));
    QScriptValue value = arg.property(QLatin1String("value"));

    if (!type.isString()) {
        qWarning() << "DeclarativeDBusInterface::typedCall - Invalid type";
        return QVariant();
    }

#ifdef QT_VERSION_5
    bool valueValid = !value.isNull() && !value.isUndefined();
#else
    bool valueValid = value.isValid();
#endif

    if (valueValid) {
        QString t = type.toString();
        if (t.length() == 1) {
            switch (t.at(0).toLatin1()) {
#ifdef QT_VERSION_5
                case 'y': return QVariant(static_cast<signed char>((value.toInt() & 0x7f) | (value.toInt() < 0 ? 0x80 : 0)));
                case 'n': return QVariant(static_cast<signed char>((value.toInt() & 0x7fff) | (value.toInt() < 0 ? 0x8000 : 0)));
                case 'q': return QVariant(static_cast<quint16>(value.toUInt()));
                case 'i': return QVariant(value.toInt());
                case 'h':
                case 'u': return QVariant(value.toUInt());
                case 'x': return QVariant(static_cast<qint64>(value.toInt()));
                case 't': return QVariant(static_cast<quint64>(value.toUInt()));
#else
                case 'y': return QVariant(static_cast<signed char>((value.toInt32() & 0x7f) | (value.toInt32() < 0 ? 0x80 : 0)));
                case 'n': return QVariant(static_cast<signed char>((value.toInt32() & 0x7fff) | (value.toInt32() < 0 ? 0x8000 : 0)));
                case 'q': return QVariant(value.toUInt16());
                case 'i': return QVariant(value.toInt32());
                case 'h':
                case 'u': return QVariant(value.toUInt32());
                case 'x': return QVariant(static_cast<qint64>(value.toInt32()));
                case 't': return QVariant(static_cast<quint64>(value.toUInt32()));
#endif
                case 'b': return QVariant(value.toBool());
                case 'd': return QVariant(static_cast<double>(value.toNumber()));
                case 's': return QVariant(value.toString());
                case 'o': return QVariant::fromValue(QDBusObjectPath(value.toString()));
                case 'g': return QVariant::fromValue(QDBusSignature(value.toString()));
                default: break;
            }
        }
        qWarning() << "DeclarativeDBusInterface::typedCall - Invalid type specifier:" << t;
    } else {
        qWarning() << "DeclarativeDBusInterface::typedCall - Invalid argument";
    }

    return QVariant();
}

QDBusMessage constructMessage(const QString &destination, const QString &path,
                              const QString &interface, const QString &method,
                              const QScriptValue &arguments)
{
    QVariantList dbusArguments;

    if (arguments.isArray()) {
#ifdef QT_VERSION_5
        quint32 len = arguments.property(QLatin1String("length")).toUInt();
#else
        quint32 len = arguments.property(QLatin1String("length")).toUInt32();
#endif
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

    QDBusMessage message = QDBusMessage::createMethodCall(destination, path, interface, method);
    message.setArguments(dbusArguments);

    return message;
}

QVariant parse(const QDBusArgument &argument)
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

}

void DeclarativeDBusInterface::typedCall(const QString &method, const QScriptValue &arguments)
{
    QDBusMessage message = constructMessage(m_destination, m_path, m_interface, method, arguments);
    if (message.type() == QDBusMessage::InvalidMessage)
        return;

    QDBusConnection conn = m_busType == SessionBus ? QDBusConnection::sessionBus()
                                                   : QDBusConnection::systemBus();

    if (!conn.send(message))
        qmlInfo(this) << conn.lastError();
}

void DeclarativeDBusInterface::typedCallWithReturn(const QString &method, const QScriptValue &arguments, const QScriptValue &callback)
{
#if QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)
    if (!callback.isCallable()) {
#else
    if (!callback.isFunction()) {
#endif
        qmlInfo(this) << "Callback argument is not a function";
        return;
    }

    QDBusMessage message = constructMessage(m_destination, m_path, m_interface, method, arguments);
    if (message.type() == QDBusMessage::InvalidMessage)
        return;

    QDBusConnection conn = m_busType == SessionBus ? QDBusConnection::sessionBus()
                                                   : QDBusConnection::systemBus();

    QDBusPendingCall pendingCall = conn.asyncCall(message);
    QDBusPendingCallWatcher *watcher = new QDBusPendingCallWatcher(pendingCall);
    connect(watcher, SIGNAL(finished(QDBusPendingCallWatcher*)),
            this, SLOT(pendingCallFinished(QDBusPendingCallWatcher*)));
    m_pendingCalls.insert(watcher, callback);
}

void DeclarativeDBusInterface::pendingCallFinished(QDBusPendingCallWatcher *watcher)
{
    QScriptValue callback = m_pendingCalls.take(watcher);

    watcher->deleteLater();

#if QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)
    if (!callback.isCallable())
#else
    if (!callback.isFunction())
#endif
        return;

    QDBusPendingReply<> reply = *watcher;

    if (reply.isError()) {
        qmlInfo(this) << reply.error();
        return;
    }

    QDBusMessage message = reply.reply();

    QScriptValueList callbackArguments;

    QVariantList arguments = message.arguments();
    foreach (QVariant argument, arguments) {
        if (argument.userType() == qMetaTypeId<QDBusArgument>())
            argument = parse(argument.value<QDBusArgument>());
        callbackArguments << callback.engine()->toScriptValue<QVariant>(argument);
    }

#if QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)
    callback.call(callbackArguments);
#else
    callback.call(QScriptValue(), callbackArguments);
#endif
}
