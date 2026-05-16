#pragma once
#include <QWidget>
#include <QSettings>
#include <QPainter>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>
struct StrengthEntry {
    int id; QString argument; QString category; QString criterion;
    qreal score; int votes; bool strong; QColor color;
};
class PaperStrengthAssessor : public QWidget {
    Q_OBJECT
public:
    explicit PaperStrengthAssessor(QWidget* parent = nullptr);
    void addEntry(const StrengthEntry& entry);
    QList<StrengthEntry> entries() const;
    int strongCount() const;
    qreal avgScore() const;
    QMap<QString, int> categoryCounts() const;
signals:
    void strengthScored(int id, qreal score);
private slots:
    void onAssess();
    void onClear();
protected:
    void paintEvent(QPaintEvent*) override;
private:
    void setupUI();
    void drawStrengthBars(QPainter& p, const QRect& rect);
    void drawCategoryChart(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();
    QList<StrengthEntry> entries_;
    QSettings settings_;
    QPushButton* assessBtn_;
    QPushButton* clearBtn_;
    QComboBox* categoryCombo_;
    QLineEdit* inputField_;
    QLabel* infoLabel_;
};
