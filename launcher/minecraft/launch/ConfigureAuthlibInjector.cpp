#include "ConfigureAuthlibInjector.h"
#include <launch/LaunchTask.h>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <Qt>

#include "Application.h"
#include "minecraft/auth/AccountList.h"
#include "minecraft/auth/TheAlteningConfig.h"
#include "minecraft/launch/TheAlteningAuthlibProxy.h"
#include "net/ChecksumValidator.h"
#include "net/Request.h"
#include "net/HttpMetaCache.h"

ConfigureAuthlibInjector::ConfigureAuthlibInjector(LaunchTask* parent,
                                                   QString authlibinjector_base_url,
                                                   std::shared_ptr<QString> javaagent_arg,
                                                   std::shared_ptr<QStringList> extra_jvm_args)
    : LaunchStep(parent),
      m_javaagent_arg{ javaagent_arg },
      m_extra_jvm_args{ extra_jvm_args },
      m_authlibinjector_base_url{ authlibinjector_base_url }
{}

void ConfigureAuthlibInjector::executeTask()
{
    auto downloadFailed = [this] (QString reason) {
        return emitFailed(QString("Download failed: %1").arg(reason));
    };
    auto entry = APPLICATION->metacache()->resolveEntry("authlibinjector", "latest.json");

    entry->setStale(true);
    m_job = std::make_unique<NetJob>("Download authlibinjector latest.json", APPLICATION->network());
    auto latestJsonDl =
        Net::Request::makeCached(QUrl("https://authlib-injector.yushi.moe/artifact/latest.json"), entry, Net::Request::Option::NoOptions);
    m_job->addNetAction(latestJsonDl);
    connect(m_job.get(), &NetJob::succeeded, this, [this, entry, downloadFailed] {
        QFile authlibInjectorLatestJson{entry->getFullPath()};
        if (!authlibInjectorLatestJson.open(QIODevice::ReadOnly))
            return emitFailed(QString("Failed to open authlib-injector info json: %1").arg(authlibInjectorLatestJson.errorString()));

        QJsonParseError json_parse_error;
        QJsonDocument doc = QJsonDocument::fromJson(authlibInjectorLatestJson.readAll(), &json_parse_error);
        if (json_parse_error.error != QJsonParseError::NoError)
            return emitFailed(QString("Failed to parse authlib-injector info json: %1").arg(json_parse_error.errorString()));

        if (!doc.isObject())
            return emitFailed(QString("Failed to parse authlib-injector info json: not a json object"));
        QJsonObject obj = doc.object();

        QString authlibInjectorJarUrl = obj["download_url"].toString();
        if (authlibInjectorJarUrl.isNull())
            return emitFailed(QString("Failed to parse authlib-injector info json: download url missing"));

        QString sha256Sum = obj["checksums"].toObject()["sha256"].toString();
        if (sha256Sum.isNull())
            return emitFailed("Failed to parse authlib-injector info json: sha256 checksum missing");

        auto sha256SumRaw = QByteArray::fromHex(sha256Sum.toLatin1());

        QString filename = QFileInfo(authlibInjectorJarUrl).fileName();
        auto javaAgentEntry = APPLICATION->metacache()->resolveEntry("authlibinjector", filename);
        m_job = std::make_unique<NetJob>("Download authlibinjector java agent", APPLICATION->network());
        auto javaAgentDl = Net::Request::makeCached(QUrl(authlibInjectorJarUrl), javaAgentEntry, Net::Request::Option::MakeEternal);
        javaAgentDl->addValidator(new Net::ChecksumValidator(QCryptographicHash::Sha256, sha256SumRaw));
        m_job->addNetAction(javaAgentDl);
        connect(m_job.get(), &NetJob::succeeded, this, [this, javaAgentEntry] {
            auto path = javaAgentEntry->getFullPath();
            qDebug() << path;

            QString agentUrl = m_authlibinjector_base_url;
            if (m_extra_jvm_args) {
                m_extra_jvm_args->clear();
            }

            if (m_authlibinjector_base_url == TheAltening::AuthlibInjectorSentinel) {
                // Keep proxy alive for the whole launch by parenting it to the LaunchTask.
                auto *proxy = new TheAlteningAuthlibProxy(m_parent);
                if (!proxy->start()) {
                    proxy->deleteLater();
                    return emitFailed(QStringLiteral("Failed to start local The Altening authlib-injector proxy."));
                }
                agentUrl = proxy->baseUrl();
                if (m_extra_jvm_args) {
                    m_extra_jvm_args->append(
                        QStringLiteral("-Dauthlibinjector.yggdrasil.prefetched=%1")
                            .arg(QString::fromLatin1(TheAlteningAuthlibProxy::prefetchedMetadataBase64())));
                    // With profileKey at default, authlib-injector installs ProfileKeyFilter and
                    // answers /player/certificates using publicKeySignature "AA==". Many servers
                    // still reject that with "Invalid signature for profile public key".
                    // "enabled" skips the dummy filter so the client does not attach that blob
                    // (The Altening tokens are not valid for Mojang's certificate API).
                    m_extra_jvm_args->append(QStringLiteral("-Dauthlibinjector.profileKey=enabled"));
                    m_extra_jvm_args->append(QStringLiteral("-Dauthlibinjector.usernameCheck=disabled"));
                }
                qDebug() << "The Altening authlib-injector proxy listening at" << agentUrl;
            }

            *m_javaagent_arg = QString("%1=%2").arg(path).arg(agentUrl);
            emitSucceeded();
        },
        // This slot can't run instantly because it needs to wait for the netjob's code to stop running
        // Since it will destroy the old netjob by reassigning the unique_ptr
        Qt::QueuedConnection);
        connect(m_job.get(), &NetJob::failed, this, downloadFailed);
        m_job->start();
    },
    // This slot can't run instantly because it needs to wait for the netjob's code to stop running
    // Since it will destroy the old netjob by reassigning the unique_ptr
    Qt::QueuedConnection);
    connect(m_job.get(), &NetJob::failed, this, downloadFailed);
    m_job->start();
}

void ConfigureAuthlibInjector::finalize() {}
