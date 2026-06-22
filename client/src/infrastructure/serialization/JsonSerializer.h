#ifndef JSONSERIALIZER_H
#define JSONSERIALIZER_H

#include <QString>
#include <QVariantMap>

class JsonSerializer
{
public:
    static QString serialize(const QVariantMap &data);
    static QVariantMap deserialize(const QString &json);
};

#endif