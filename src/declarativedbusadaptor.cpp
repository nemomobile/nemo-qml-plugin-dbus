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

#include "declarativedbusinterface.h"

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

template <typename T> QVariant castList(const QVariantList &list)
{
    QDBusArgument argument;
    argument.beginArray(qMetaTypeId<T>());
    foreach (const QVariant &item, list) {
        argument << item.value<T>();
    }
    argument.endArray();
    return QVariant::fromValue(argument);
}

static QVariant readProperty(QObject *object, const QMetaProperty &property)
{
    QVariant value = property.read(object);

    if (value.type() == QVariant::List) {
        QVariantList variantList = value.toList();
        if (variantList.count() > 0) {
            switch (variantList.first().type()) {
            case QVariant::String:      value = castList<QString>(variantList); break;
            case QVariant::Int:         value = castList<int>(variantList); break;
            case QVariant::UInt:        value = castList<uint>(variantList); break;
            case QVariant::Bool:        value = castList<bool>(variantList); break;
            case QVariant::Double:      value = castList<double>(variantList); break;
            default: break;
            }
        }
#ifdef QT_VERSION_5
    } else if (value.userType() == qMetaTypeId<QJSValue>()) {
        value = DeclarativeDBusInterface::marshallDBusArgument(value.value<QJSValue>());
#endif
    }

    return value;
}

static QVariant readTypedProperty(QObject *object, const QMetaProperty &property, const QString &type)
{
    QVariant value = property.read(object);
    if (type.length() == 1) {
        switch (type.at(0).toLatin1()) {
            case 'y': return QVariant(static_cast<signed char>((value.toInt() & 0x7f) | (value.toInt() < 0 ? 0x80 : 0)));
            case 'n': return QVariant(static_cast<qint16>((value.toInt() & 0x7fff) | (value.toInt() < 0 ? 0x8000 : 0)));
            case 'q': return QVariant(static_cast<quint16>(value.toUInt()));
            case 'i': return QVariant(value.toInt());
            case 'h':
            case 'u': return QVariant(value.toUInt());
            case 'x': return QVariant(static_cast<qint64>(value.toInt()));
            case 't': return QVariant(static_cast<quint64>(value.toUInt()));
            case 'v': return value;
            case 'b': return QVariant(value.toBool());
            case 'd': return QVariant(static_cast<double>(value.toDouble()));
            case 's': return QVariant(value.toString());
            case 'o': return QVariant::fromValue(QDBusObjectPath(value.toString()));
            case 'g': return QVariant::fromValue(QDBusSignature(value.toString()));
            default: break;
        }
    } else if (type.length() == 2 && (type.at(0).toLatin1() == 'a')) {
        // The result must be an array of typed data
        if (!value.type() != QVariant::List) {
            qWarning() << "Invalid value for type specifier:" << type << "v:" << value;
        }  else {
            const QVariantList list = value.toList();
            switch (type.at(1).toLatin1()) {
                case 's': return QVariant(castList<QString>(list));
                default: break;
            }
        }
    }
    qWarning() << "DeclarativeDBusInterface::typedCall - Invalid type specifier:" << type;
    return QVariant();
}

bool DeclarativeDBusAdaptor::handleMessage(const QDBusMessage &message, const QDBusConnection &connection)
{
    QVariant variants[10];
    QGenericArgument arguments[10];

    const QMetaObject * const meta = metaObject();
    const QVariantList dbusArguments = message.arguments();

    QString member = message.member();
    QString interface = message.interface();

    // Don't try to handle introspect call. It will be handled
    // internally for QDBusVirtualObject derived classes.
    if (interface == QLatin1String("org.freedesktop.DBus.Introspectable")) {
        return false;
    } else if (interface == QLatin1String("org.freedesktop.DBus.Properties")) {
        if (member == QLatin1String("Get")) {
            interface = dbusArguments.value(0).toString();
            member = dbusArguments.value(1).toString();

            const QMetaObject * const meta = metaObject();
            if (!member.isEmpty() && member.at(0).isUpper())
                member = "rc" + member;

            for (int propertyIndex = meta->propertyOffset();
                        propertyIndex < meta->propertyCount();
                        ++propertyIndex) {
                QMetaProperty property = meta->property(propertyIndex);

                if (QLatin1String(property.name()) != member)
                    continue;


                QVariant value;
                const int typeIndex = meta->indexOfProperty(QByteArray("typeinfo_") + property.name());
                if (typeIndex >= 0) {
                    value = readTypedProperty(
                                this, property, meta->property(typeIndex).read(this).toString());
                } else {
                    value = readProperty(this, property);
                }

                QDBusMessage reply = message.createReply(QVariantList() << value);
                connection.call(reply, QDBus::NoBlock);

                return true;
            }
        } else if (member == QLatin1String("GetAll")) {
            interface = dbusArguments.value(0).toString();

            QVariantMap map;

            for (int propertyIndex = meta->propertyOffset();
                        propertyIndex < meta->propertyCount();
                        ++propertyIndex) {
                QMetaProperty property = meta->property(propertyIndex);

                QString propertyName = QLatin1String(property.name());

                if (QByteArray(property.name()).startsWith("typeinfo_"))
                    continue;

                if (propertyName.startsWith(QLatin1String("rc")))
                    propertyName = propertyName.mid(2);

                QVariant value;
                const int typeIndex = meta->indexOfProperty(QByteArray("typeinfo_") + property.name());
                if (typeIndex >= 0) {
                    value = readTypedProperty(
                                this, property, meta->property(typeIndex).read(this).toString());
                } else {
                    value = readProperty(this, property);
                }

                map.insert(propertyName, value);
            }

            QDBusMessage reply = message.createReply(QVariantList() << map);
            connection.call(reply, QDBus::NoBlock);

            return true;
        } else if (member == QLatin1String("Set")) {
            interface = dbusArguments.value(0).toString();
            member = dbusArguments.value(1).toString();
            QVariant value = dbusArguments.value(3);

            const QMetaObject * const meta = metaObject();
            if (!member.isEmpty() && member.at(0).isUpper())
                member = "rc" + member;

            for (int propertyIndex = meta->propertyOffset();
                        propertyIndex < meta->propertyCount();
                        ++propertyIndex) {
                QMetaProperty property = meta->property(propertyIndex);

                if (QLatin1String(property.name()) != member)
                    continue;

                if (value.userType() == qMetaTypeId<QDBusArgument>())
                    value = DeclarativeDBusInterface::parse(value.value<QDBusArgument>());

                return property.write(this, value);
            }
        }
        return false;
    }

    // Support interfaces with method names starting with an uppercase letter.
    // com.example.interface.Foo -> com.example.interface.rcFoo (remote-call Foo).
    if (!member.isEmpty() && member.at(0).isUpper())
        member = "rc" + member;

    for (int methodIndex = meta->methodOffset(); methodIndex < meta->methodCount(); ++methodIndex) {
        const QMetaMethod method = meta->method(methodIndex);
        const QList<QByteArray> parameterTypes = method.parameterTypes();

        if (parameterTypes.count() != dbusArguments.count())
            continue;

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
                argument = DeclarativeDBusInterface::parse(argument.value<QDBusArgument>());
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
            if (method.methodType() == QMetaMethod::Signal) {
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
            } else {
                QVariant retVal;
                bool success = method.invoke(
                            this,
                            Q_RETURN_ARG(QVariant, retVal),
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
                if (retVal.isValid()) {
                    QDBusMessage reply = message.createReply(retVal);
                    QDBusConnection conn = m_busType == SessionBus ? QDBusConnection::sessionBus()
                                                                   : QDBusConnection::systemBus();
                    conn.send(reply);
                }
                return success;
            }
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

void DeclarativeDBusAdaptor::emitSignalWithArguments(
        const QString &name, const QScriptValue &arguments)
{
    QDBusMessage signal = QDBusMessage::createSignal(m_path, m_interface, name);
    signal.setArguments(DeclarativeDBusInterface::argumentsFromScriptValue(arguments));
    QDBusConnection conn = m_busType == SessionBus ? QDBusConnection::sessionBus()
                                                   : QDBusConnection::systemBus();

    if (!conn.send(signal))
        qmlInfo(this) << conn.lastError();
}

void DeclarativeDBusAdaptor::emitSignalWithTypedArguments(
        const QString &name, const QScriptValue &arguments)
{
    QDBusMessage signal = QDBusMessage::createSignal(m_path, m_interface, name);
    signal.setArguments(DeclarativeDBusInterface::marshallDBusArguments(arguments));
    QDBusConnection conn = m_busType == SessionBus ? QDBusConnection::sessionBus()
                                                   : QDBusConnection::systemBus();

    if (!conn.send(signal))
        qmlInfo(this) << conn.lastError();
}
