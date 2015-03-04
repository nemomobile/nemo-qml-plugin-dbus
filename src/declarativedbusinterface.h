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

#ifndef DECLARATIVEDBUSINTERFACE_H
#define DECLARATIVEDBUSINTERFACE_H

#include <QObject>
#include <QMap>
#include <QPair>
#include <QPointer>
#include <QVariant>
#include <QDBusArgument>
#include <QJSValue>
#include <QQmlParserStatus>
#include <QUrl>
#include <QDBusPendingCallWatcher>
#include <QDBusMessage>
#include <QPair>

#include "declarativedbus.h"

class DeclarativeDBusInterface : public QObject, public QQmlParserStatus
{
    Q_OBJECT

    Q_PROPERTY(QString service READ service WRITE setService NOTIFY serviceChanged)
    Q_PROPERTY(QString path READ path WRITE setPath NOTIFY pathChanged)
    Q_PROPERTY(QString iface READ interface WRITE setInterface NOTIFY interfaceChanged)
    Q_PROPERTY(DeclarativeDBus::BusType bus READ bus WRITE setBus NOTIFY busChanged)
    Q_PROPERTY(bool signalsEnabled READ signalsEnabled WRITE setSignalsEnabled NOTIFY signalsEnabledChanged)

    Q_INTERFACES(QQmlParserStatus)

public:
    DeclarativeDBusInterface(QObject *parent = 0);
    ~DeclarativeDBusInterface();

    QString service() const;
    void setService(const QString &service);

    QString path() const;
    void setPath(const QString &path);

    QString interface() const;
    void setInterface(const QString &interface);

    DeclarativeDBus::BusType bus() const;
    void setBus(DeclarativeDBus::BusType bus);

    bool signalsEnabled() const;
    void setSignalsEnabled(bool enabled);

    Q_INVOKABLE void call(const QString &method, const QJSValue &arguments);
    Q_INVOKABLE bool typedCall(const QString &method, const QJSValue &arguments,
            const QJSValue &callback=QJSValue::UndefinedValue,
            const QJSValue &errorCallback=QJSValue::UndefinedValue);

    Q_INVOKABLE QVariant getProperty(const QString &name);
    Q_INVOKABLE void setProperty(const QString &name, const QVariant &value);

    void classBegin();
    void componentComplete();

    static QVariant parse(const QDBusArgument &argument);
    static QVariantList argumentsFromScriptValue(const QJSValue &arguments);

signals:
    void serviceChanged();
    void pathChanged();
    void interfaceChanged();
    void busChanged();
    void signalsEnabledChanged();

private slots:
    void pendingCallFinished(QDBusPendingCallWatcher *watcher);
    void signalHandler(const QDBusMessage &message);
    void connectSignalHandlerCallback(const QString &introspectionData);

private:
    void disconnectSignalHandler();
    void connectSignalHandler();

    QString m_service;
    QString m_path;
    QString m_interface;
    DeclarativeDBus::BusType m_bus;
    QMap<QDBusPendingCallWatcher *, QPair<QJSValue, QJSValue> > m_pendingCalls; // pair: success and error callback
    QMap<QString, QMetaMethod> m_signals;
    bool m_componentCompleted;
    bool m_signalsEnabled;
};

#endif
