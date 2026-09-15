#pragma once

#include <launch/LaunchStep.h>
#include <minecraft/auth/MinecraftAccount.h>
#include <QStringList>
#include "net/NetJob.h"

class ConfigureAuthlibInjector: public LaunchStep
{
    Q_OBJECT
public:
    explicit ConfigureAuthlibInjector(LaunchTask *parent,
                                      QString authlibinjector_base_url,
                                      std::shared_ptr<QString> javaagent_arg,
                                      std::shared_ptr<QStringList> extra_jvm_args = nullptr);
    virtual ~ConfigureAuthlibInjector() {};

    void executeTask() override;
    void finalize() override;
    bool canAbort() const override
    {
        return false;
    }
private:
    std::unique_ptr<NetJob> m_job;
    std::shared_ptr<QString> m_javaagent_arg;
    std::shared_ptr<QStringList> m_extra_jvm_args;
    QString m_authlibinjector_base_url;
};
