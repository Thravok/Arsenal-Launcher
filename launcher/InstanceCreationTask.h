#pragma once

#include "InstanceTask.h"

class InstanceCreationTask : public InstanceTask {
    Q_OBJECT
   public:
    InstanceCreationTask();
    ~InstanceCreationTask() override = default;

   protected:
    void executeTask() final override;

    /**
     * Tries to update an already existing instance.
     *
     * If this returns true, createInstance() will not run.
     */
    virtual bool updateInstance() { return false; }

    /**
     * Creates a new instance. Returns whether creation succeeded.
     */
    virtual bool createInstance() { return false; }

    QString getError() const { return m_error_message; }

    void setError(QString message) { m_error_message = message; }

   protected:
    bool m_abort = false;
    QStringList m_files_to_remove;

   private:
    QString m_error_message;
};
