#pragma once

#include <QWidget>
#include <QLabel>
#include <QPushButton>
#include <QComboBox>
#include <QTreeWidget>
#include <QTextEdit>
#include <QLineEdit>
#include <QList>
#include <QMap>
#include <QSettings>

struct CrossRef {
    int id{-1};
    int sourcePaperId{-1};
    int targetPaperId{-1};
    QString sourceTitle;
    QString targetTitle;
    QString refType;
    QString context;
    QString notes;
    qreal strength{1.0};
};

class PaperCrossReference : public QWidget {
    Q_OBJECT

public:
    explicit PaperCrossReference(QWidget* parent = nullptr);

    void setPapers(const QList<QPair<int, QString>>& papers);
    void addRef(const CrossRef& ref);
    void removeRef(int refId);
    QList<CrossRef> refs() const;
    QList<CrossRef> refsForPaper(int paperId) const;
    QList<CrossRef> refsByType(const QString& type) const;
    QMap<int, int> buildAdjacency() const;

signals:
    void refAdded(int sourceId, int targetId, const QString& type);
    void refRemoved(int refId);
    void paperClicked(int paperId);

private slots:
    void onAdd();
    void onDelete();
    void onFilterChanged(int index);
    void onRefSelected();
    void onAutoDetect();
    void onExport();

private:
    void setupUI();
    void refreshTree();
    void updateStats();
    void loadSettings();
    void saveSettings();

    QTreeWidget* refTree_{nullptr};
    QTextEdit* contextEdit_{nullptr};
    QComboBox* sourceCombo_{nullptr};
    QComboBox* targetCombo_{nullptr};
    QComboBox* typeCombo_{nullptr};
    QComboBox* filterCombo_{nullptr};
    QPushButton* addBtn_{nullptr};
    QPushButton* deleteBtn_{nullptr};
    QPushButton* autoBtn_{nullptr};
    QPushButton* exportBtn_{nullptr};
    QLabel* statsLabel_{nullptr};

    QList<CrossRef> refs_;
    QList<QPair<int, QString>> papers_;
    int nextId_{1};
    int selectedRefId_{-1};
};
