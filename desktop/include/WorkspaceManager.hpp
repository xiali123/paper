#pragma once

#include <QWidget>
#include <QListWidget>
#include <QLabel>
#include <QPushButton>
#include <QMap>
#include <QJsonObject>

struct WorkspaceLayout {
    QString name;
    qint64 createdAt{0};
    int activeTab{0};
    QByteArray windowGeometry;
    QByteArray splitterState;
    QMap<QString, QVariant> extraState;
};

class WorkspaceManager : public QWidget {
    Q_OBJECT

public:
    explicit WorkspaceManager(QWidget* parent = nullptr);

    void captureCurrentLayout(const QString& name, int activeTab,
                              const QByteArray& geometry, const QByteArray& splitter);
    WorkspaceLayout currentLayout() const;

    QList<WorkspaceLayout> savedLayouts() const;
    void loadLayouts();
    void saveLayouts();

signals:
    void layoutSaved(const QString& name);
    void layoutRestored(const WorkspaceLayout& layout);
    void layoutDeleted(const QString& name);

private slots:
    void onSaveLayout();
    void onRestoreLayout();
    void onDeleteLayout();
    void onRenameLayout();

private:
    void setupUI();
    void refreshList();

    QListWidget* layoutList_{nullptr};
    QLabel* infoLabel_{nullptr};
    QPushButton* saveBtn_{nullptr};
    QPushButton* restoreBtn_{nullptr};
    QPushButton* deleteBtn_{nullptr};

    QList<WorkspaceLayout> layouts_;
    int nextId_{0};

    static constexpr int MAX_LAYOUTS = 20;
};
