#include "sqlvalue.h"

QString SqlValue::serialize(const QVariant &property)
{
    return property.toString();
}

QVariant SqlValue::deserialize(const QVariant::Type &fieldType, const QVariant &value)
{
    Q_UNUSED(fieldType)
    return value;
}
