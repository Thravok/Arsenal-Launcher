#pragma once

#include <QObject>
#include <QJsonObject>
#include <QString>
#include <QNetworkReply>

#include "QObjectPtr.h"

/**
 * Thin GET-only client for The Altening panel API (https://api.thealtening.com/v2/).
 * Never logs API keys or alt tokens.
 */
class TheAlteningApi : public QObject {
    Q_OBJECT
public:
    using Ptr = shared_qobject_ptr<TheAlteningApi>;

    explicit TheAlteningApi(QObject *parent = nullptr);

    void checkLicense(const QString &apiKey);
    void generate(const QString &apiKey, bool withInfo = true);
    void info(const QString &apiKey, const QString &token);

    static QString errorMessageForStatus(int httpStatus);

signals:
    void succeeded(QJsonObject json);
    void failed(QString reason);

private slots:
    void onReplyFinished();

private:
    void get(const QString &path, const QList<QPair<QString, QString>> &query);

    QNetworkReply *m_reply = nullptr;
};
