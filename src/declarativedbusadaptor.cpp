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

#include "declarativedbusadaptor.h"

#include <QDBusArgument>
#include <QDBusMessage>
#include <QDBusConnection>
#include <QMetaMethod>

#ifdef QT_VERSION_5
#include <qqmlinfo.h>
#else
#include <qdeclarativeinfo.h>
#endif
#include <QtDebug>

DeclarativeDBusAdaptor::DeclarativeDBusAdaptor(QObject *parent)
    : QDBusVirtualObject(parent), m_busType(SessionBus)
{
}

DeclarativeDBusAdaptor::~DeclarativeDBusAdaptor()
{
}

QString DeclarativeDBusAdaptor::service() const
{
    return m_service;
}

void DeclarativeDBusAdaptor::setService(const QString &service)
{
    if (m_service != service) {
        m_service = service;
        emit serviceChanged();
    }
}

QString DeclarativeDBusAdaptor::path() const
{
    return m_path;
}

void DeclarativeDBusAdaptor::setPath(const QString &path)
{
    if (m_path != path) {
        m_path = path;
        emit pathChanged();
    }
}

QString DeclarativeDBusAdaptor::interface() const
{
    return m_interface;
}

void DeclarativeDBusAdaptor::setInterface(const QString &interface)
{
    if (m_interface != interface) {
        m_interface = interface;
        emit interfaceChanged();
    }
}

/*
    The XML service description.  This could be derived from the meta-object but since its unlikely
    to be needed most of the time this and type conversion is non-trivial it's not, and there is
    this property instead if it is needed.
*/

QString DeclarativeDBusAdaptor::xml() const
{
    return m_xml;
}

void DeclarativeDBusAdaptor::setXml(const QString &xml)
{
    if (m_xml != xml) {
        m_xml = xml;
        emit xmlChanged();
    }
}

DeclarativeDBusAdaptor::BusType DeclarativeDBusAdaptor::busType() const
{
    return m_busType;
}

void DeclarativeDBusAdaptor::setBusType(DeclarativeDBusAdaptor::BusType busType)
{
    if (m_busType != busType) {
        m_busType = busType;
        emit busTypeChanged();
    }
}

void DeclarativeDBusAdaptor::classBegin()
{
}

void DeclarativeDBusAdaptor::componentComplete()
{
    QDBusConnection conn = m_busType == SessionBus ? QDBusConnection::sessionBus()
                                                   : QDBusConnection::systemBus();

    // Register service name only if it has been set.
    if (!m_service.isEmpty()) {
        if (!conn.registerService(m_service)) {
            qmlInfo(this) << "Failed to register service" << m_service;
            qmlInfo(this) << conn.lastError().message();
        }
    }

    // It is still valid to publish an object on the bus without first registering a service name,
    // a remote process would have to connect directly to the DBus address.
    if (!conn.registerVirtualObject(m_path, this)) {
        qmlInfo(this) << "Failed to register object" << m_path;
        qmlInfo(this) << conn.lastError().message();
    }
}

QString DeclarativeDBusAdaptor::introspect(const QString &) const
{
    return m_xml;
}

static QVariant parse(const QDBusArgument &argument)
{
    if (argument.currentType() == QDBusArgument::ArrayType) {
        QVariantList list;
        argument >> list;
        return list;
    } else if (argument.currentType() == QDBusArgument::MapType) {
        QVariantMap map;
        argument >> map;
        return map;
    } else {
        return QVariant();
    }
}

bool DeclarativeDBusAdaptor::handleMessage(const QDBusMessage &message, const QDBusConnection &)
{
    QVariant variants[10];
    QGenericArgument arguments[10];

    const QMetaObject * const meta = metaObject();
    const QVariantList dbusArguments = message.arguments();

    for (int methodIndex = meta->methodOffset(); methodIndex < meta->methodCount(); ++methodIndex) {
        const QMetaMethod method = meta->method(methodIndex);
        const QList<QByteArray> parameterTypes = method.parameterTypes();

        if (parameterTypes.count() != dbusArguments.count())
            continue;

        QString member = message.member();
        // Don't try to handle introspect call. It will be handled
        // internally for QDBusVirtualObject derived classes.
        if (member == QLatin1String("Introspect")) {
            return false;
        }
        // Support interfaces with method names starting with an uppercase letter.
        // com.example.interface.Foo -> com.example.interface.rcFoo (remote-call Foo).
        if (!member.isEmpty() && member.at(0).isUpper())
            member = "rc" + member;

#ifdef QT_VERSION_5
        QByteArray sig(method.methodSignature());
#else
        QByteArray sig(method.signature());
#endif

        if (!sig.startsWith(member.toLatin1() + "("))
            continue;

        int argumentCount = 0;
        for (; argumentCount < 10 && argumentCount < dbusArguments.count(); ++argumentCount) {
            variants[argumentCount] = dbusArguments.at(argumentCount);
            QVariant &argument = variants[argumentCount];

            // If the variant type is a QBusArgument attempt to parse its contents.
            if (argument.userType() == qMetaTypeId<QDBusArgument>()) {
                argument = parse(argument.value<QDBusArgument>());
            }

            const QByteArray parameterType = parameterTypes.at(argumentCount);
            if (parameterType == "QVariant") {
                arguments[argumentCount] = QGenericArgument("QVariant", &argument);
            } else if (parameterType == argument.typeName()) {
                arguments[argumentCount] = QGenericArgument(argument.typeName(), argument.data());
            } else if (parameterType == "QString" && argument.userType() == qMetaTypeId<QDBusObjectPath>()) {
                // QDBusObjectPath is not exported to QML, use QString instead.
                arguments[argumentCount] = Q_ARG(QString, argument.value<QDBusObjectPath>().path());
            } else if (parameterType == "QString" && argument.userType() == qMetaTypeId<QDBusSignature>()) {
                // QDBusSignature is not exported to QML, use QString instead.
                arguments[argumentCount] = Q_ARG(QString, argument.value<QDBusSignature>().signature());
            } else {
                // Type mismatch, there may be another overload.
                break;
            }
        }

        if (argumentCount == dbusArguments.length()) {
            return method.invoke(
                    this,
                    arguments[0],
                    arguments[1],
                    arguments[2],
                    arguments[3],
                    arguments[4],
                    arguments[5],
                    arguments[6],
                    arguments[7],
                    arguments[8],
                    arguments[9]);
        }
    }
    QByteArray signature = message.member().toLatin1() + "(";
    for (int i = 0; i < dbusArguments.count(); ++i) {
        if (i > 0)
            signature += ",";
        signature += dbusArguments.at(i).typeName();
    }
    signature += ")";

    qmlInfo(this) << "No method with the signature " << signature;
    return false;
}


void DeclarativeDBusAdaptor::emitSignal(const QString &name)
{
    QDBusMessage signal = QDBusMessage::createSignal(m_path, m_interface, name);
    QDBusConnection conn = m_busType == SessionBus ? QDBusConnection::sessionBus()
                                                   : QDBusConnection::systemBus();

    if (!conn.send(signal))
        qmlInfo(this) << conn.lastError();
}

