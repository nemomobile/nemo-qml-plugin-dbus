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
#ifdef QT_VERSION_5
# include <qqmlinfo.h>
# include <QJSValue>
# include <QJSValueIterator>
#else
# include <QScriptEngine>
# include <QScriptValue>
# include <qdeclarativeinfo.h>
#endif
#include <QFile>
#include <QUrl>

#include <QtDebug>

DeclarativeDBusInterface::DeclarativeDBusInterface(QObject *parent)
    : QObject(parent), m_busType(SessionBus)
{
}

DeclarativeDBusInterface::~DeclarativeDBusInterface()
{
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

}

void DeclarativeDBusInterface::typedCall(const QString &method, const QScriptValue &arguments)
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
                return;
            }
            dbusArguments.append(value);
        }
    } else if (!arguments.isUndefined()) {
        // arguments is a singular typed value
        QVariant value = marshallDBusArgument(arguments);
        if (!value.isValid()) {
            return;
        }
        dbusArguments.append(value);
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
