#pragma once

#include <QDialog>
#include <QTreeWidgetItem>

namespace Ui {
class ReviewMessageBox;
}

class ReviewMessageBox : public QDialog {
    Q_OBJECT

   public:
    static auto create(QWidget* parent, QString&& title, QString&& icon = "") -> ReviewMessageBox*;

    using ResourceInformation = struct res_info {
        QString name;
        QString filename;
        QString provider;
        QStringList required_by;
        /** Version number / label (e.g. 1.2.3). Shown as "Version: …". */
        QString version;
        /** Channel / type (e.g. release, beta). Shown as "Version Type: …". */
        QString version_type;
        bool enabled = true;
        /** If set, returned by deselectedResources() instead of name (e.g. projectId). */
        QString deselectKey;
        /** Optional tooltip on the resource row (overrides the default disabled tip). */
        QString tooltip;
    };

    void appendResource(ResourceInformation&& info);
    auto deselectedResources() -> QStringList;

    void retranslateUi(QString resources_name);

    ~ReviewMessageBox() override;

   protected slots:
    void on_toggleDepsButton_clicked();

   protected:
    ReviewMessageBox(QWidget* parent, const QString& title, const QString& icon);

    Ui::ReviewMessageBox* ui;

    QList<QTreeWidgetItem*> m_deps;
    bool m_deps_checked = true;
};
