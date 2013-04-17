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
#include <QScriptEngine>
#include <QScriptValue>
#include <QFile>
#include <QUrl>

#include <qdeclarativeinfo.h>
#include <QtDebug>

DeclarativeDBusInterface::DeclarativeDBusInterface(QObject *parent)
    : QObject(parent)
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

void DeclarativeDBusInterface::call(const QString &method, const QScriptValue &arguments)
{
    QVariantList dbusArguments;

    if (arguments.isArray()) {
        qScriptValueToSequence(arguments, dbusArguments);
    } else if (!arguments.isUndefined()) {
        dbusArguments.append(qscriptvalue_cast<QVariant>(arguments));
    }

    QDBusMessage message = QDBusMessage::createMethodCall(
                m_destination,
                m_path,
                m_interface,
                method);
    message.setArguments(dbusArguments);
    if (!QDBusConnection::sessionBus().send(message)) {
        qmlInfo(this) << QDBusConnection::systemBus().lastError();
    }
}

namespace {

QVariant marshallDBusArgument(const QScriptValue &arg)
{
    QScriptValue type = arg.property(QLatin1String("type"));
    QScriptValue value = arg.property(QLatin1String("value"));

    if (type.isValid() && value.isValid()) {
        QString t = type.toString();
        if (t.length() == 1) {
            switch (t.at(0).toAscii()) {
                case 'y': return QVariant(static_cast<signed char>((value.toInt32() & 0x7f) | (value.toInt32() < 0 ? 0x80 : 0)));
                case 'b': return QVariant(value.toBool());
                case 'n': return QVariant(static_cast<signed char>((value.toInt32() & 0x7fff) | (value.toInt32() < 0 ? 0x8000 : 0)));
                case 'q': return QVariant(value.toUInt16());
                case 'i': return QVariant(value.toInt32());
                case 'h':
                case 'u': return QVariant(value.toUInt32());
                case 'x': return QVariant(static_cast<qint64>(value.toInt32()));
                case 't': return QVariant(static_cast<quint64>(value.toUInt32()));
                case 'd': return QVariant(static_cast<double>(value.toNumber()));
                case 's':
                case 'o':
                case 'g': return QVariant(value.toString());
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
        quint32 len = arguments.property(QLatin1String("length")).toUInt32();
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
    if (!QDBusConnection::sessionBus().send(message)) {
        qmlInfo(this) << QDBusConnection::systemBus().lastError();
    }
}
