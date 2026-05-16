#pragma once
#include <QWidget>
#include <QSettings>
#include <QPainter>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>
struct ReadingEndurance2Entry {
    int id; QString session; QString category; QString mode;
    qreal stamina; int pages; bool marathon; QColor color;
};
class PaperReadingEndurance2 : public QWidget {
    Q_OBJECT
public:
    explicit PaperReadingEndurance2(QWidget* parent = nullptr);
    void addEntry(const ReadingEndurance2Entry& entry);
    QList<ReadingEndurance2Entry> entries() const;
    int marathonCount() const;
    qreal avgStamina() const;
    QMap<QString, int> categoryCounts() const;
signals:
    void sessionComplete(int id, qreal stamina);
private slots:
    void onStart();
    void onClear();
protected:
    void paintEvent(QPaintEvent*) override;
private:
    void setupUI();
    void drawEnduranceView(QPainter& p, const QRect& rect);
    void drawCategoryChart(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();
    QList<ReadingEndurance2Entry> entries_;
    QSettings settings_;
    QPushButton* startBtn_;
    QPushButton* clearBtn_;
    QComboBox* categoryCombo_;
    QLineEdit* inputField_;
    QLabel* infoLabel_;
};
