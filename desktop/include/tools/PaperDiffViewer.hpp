#pragma once
#include <QWidget>
#include <QSettings>
#include <QPainter>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>

struct DiffEntry {
    int id;
    QString file;
    QString category;
    QString change;
    int added;
    int removed;
    QString author;
    bool conflict;
    QColor color;
};

class PaperDiffViewer : public QWidget {
    Q_OBJECT
public:
    explicit PaperDiffViewer(QWidget* parent = nullptr);
    void addEntry(const DiffEntry& entry);
    QList<DiffEntry> entries() const;
    int conflictCount() const;
    int totalAdded() const;
    QMap<QString, int> categoryCounts() const;
signals:
    void diffLoaded(int id, int added);
private slots:
    void onLoad();
    void onClear();
protected:
    void paintEvent(QPaintEvent*) override;
private:
    void setupUI();
    void drawDiffList(QPainter& p, const QRect& rect);
    void drawCategoryChart(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();
    QList<DiffEntry> entries_;
    QSettings settings_;
    QPushButton* loadBtn_;
    QPushButton* clearBtn_;
    QComboBox* categoryCombo_;
    QLineEdit* inputField_;
    QLabel* infoLabel_;
};
