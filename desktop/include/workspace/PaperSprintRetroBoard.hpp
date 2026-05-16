#pragma once
#include <QWidget>
#include <QSettings>
#include <QPainter>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>
struct SprintRetroEntry {
    int id; QString title; QString category; QString type;
    qreal votes; int actionItems; bool addressed; QColor color;
};
class PaperSprintRetroBoard : public QWidget {
    Q_OBJECT
public:
    explicit PaperSprintRetroBoard(QWidget* parent = nullptr);
    void addEntry(const SprintRetroEntry& entry);
    QList<SprintRetroEntry> entries() const;
    int addressedCount() const;
    qreal avgVotes() const;
    QMap<QString, int> categoryCounts() const;
signals:
    void retroSaved(int id, qreal votes);
private slots:
    void onAdd();
    void onClear();
protected:
    void paintEvent(QPaintEvent*) override;
private:
    void setupUI();
    void drawRetroBoard(QPainter& p, const QRect& rect);
    void drawCategoryChart(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();
    QList<SprintRetroEntry> entries_;
    QSettings settings_;
    QPushButton* addBtn_;
    QPushButton* clearBtn_;
    QComboBox* categoryCombo_;
    QLineEdit* inputField_;
    QLabel* infoLabel_;
};
