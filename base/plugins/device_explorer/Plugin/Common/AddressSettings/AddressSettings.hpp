#pragma once

#include <QString>
#include <QVariant>

struct AddressSettings
{
    QString name;
    QString valueType;
    int priority;
    QString tags;
    QString ioType;
    QVariant addressSpecificSettings;
    QVariant value;
};

