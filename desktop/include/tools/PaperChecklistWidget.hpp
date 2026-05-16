#pragma once

#include <QWidget>
#include <QTreeWidget>
#include <QLabel>
#include <QPushButton>
#include <QProgressBar>
#include <QMap>
#include <QList>
#include <QSettings>

struct CheckItem {
    int id{-1};
    QString text;
    bool checked{false};
    QString category;
    int priority{0};
};

struct PaperChecklist {
    int paperId{-1};
    QString paperTitle;
    QList<CheckItem> items;
    qint64 createdAt{0};
    qint64 completedAt{0};
    double completionPct() const;
};

class PaperChecklistWidget : public QWidget {
    Q_OBJECT

public:
    explicit PaperChecklistWidget(QWidget* parent = nullptr);

    void setPaper(int paperId, const QString& title);
    void addDefaultItems();
    void addItem(const QString& text, const QString& category = "General", int priority = 0);
    void setChecked(int itemId, bool checked);
    QList<PaperChecklist> allChecklists() const;
    int totalCompleted() const;

signals:
    void itemChecked(int paperId, int itemId, bool checked);
    void checklistCompleted(int paperId);
    void progressChanged(int paperId, double pct);

private slots:
    void onItemChanged(QTreeWidgetItem* item, int column);
    void onAddCustom();
    void onRemove();
    void onReset();
    void onClearDone();

private:
    void setupUI();
    void loadSettings();
    void saveSettings();
    void refreshTree();
    void updateProgress();

    QTreeWidget* tree_{nullptr};
    QProgressBar* progressBar_{nullptr};
    QLabel* paperLabel_{nullptr};
    QLabel* statsLabel_{nullptr};
    QPushButton* addBtn_{nullptr};
    QPushButton* removeBtn_{nullptr};
    QPushButton* resetBtn_{nullptr};
    QPushButton* clearDoneBtn_{nullptr};

    QMap<int, PaperChecklist> checklists_;
    int currentPaperId_{-1};
    int nextItemId_{1};
};
