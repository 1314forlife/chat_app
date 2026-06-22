#include "JsonSerializer.h"
#include <QJsonDocument>
#include <QJsonObject>

QString JsonSerializer::serialize(const QVariantMap &data)
{
    QJsonObject obj = QJsonObject::fromVariantMap(data);
    return QString::fromUtf8(QJsonDocument(obj).toJson());
}

QVariantMap JsonSerializer::deserialize(const QString &json)
{
    QJsonDocument doc = QJsonDocument::fromJson(json.toUtf8());
    return doc.object().toVariantMap();
}