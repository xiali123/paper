#pragma once
#include <QWidget>
#include <QSettings>
#include <QPushButton>
#include <QLabel>
#include <QLineEdit>
#include <QComboBox>

struct RenameEntry {
    int id;
    QString originalName;
    QString newName;
    QString pattern;
    QString category;
    int filesAffected;
    QString status;
    QString preview;
    QColor color;
};

class PaperBatchRenamer : public QWidget {
    Q_OBJECT
public:
    explicit PaperBatchRenamer(QWidget* parent = nullptr);
    void addEntry(const RenameEntry& entry);
    QList<RenameEntry> entries() const;
    QMap<QString, int> statusCounts() const;
    int totalFiles() const;
    int renamedCount() const;

signals:
    void renameComplete(int id, int filesAffected);

protected:
    void paintEvent(QPaintEvent*) override;

private:
    void setupUI();
    void onRename();
    void onClear();
    void drawRenameList(QPainter& p, const QRect& rect);
    void drawStatusChart(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();

    QSettings settings_;
    QList<RenameEntry> entries_;
    QPushButton* renameBtn_;
    QPushButton* clearBtn_;
    QLineEdit* patternField_;
    QComboBox* strategyCombo_;
    QLabel* infoLabel_;
};
