#ifndef SQLVALUE_H
#define SQLVALUE_H

#include <QVariant>

class SqlValue
{
public:
    static QString serialize(const QVariant &property);
    static QVariant deserialize(const QVariant::Type &fieldType, const QVariant &value);
};

#endif // SQLVALUE_H
