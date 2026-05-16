#pragma once
#include <QWidget>
#include <QSettings>
#include <QPainter>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>
struct ReviewExchange2Entry {
    int id; QString paper; QString category; QString reviewer;
    qreal quality; int comments; bool constructive; QColor color;
};
class PaperReviewExchange2 : public QWidget {
    Q_OBJECT
public:
    explicit PaperReviewExchange2(QWidget* parent = nullptr);
    void addEntry(const ReviewExchange2Entry& entry);
    QList<ReviewExchange2Entry> entries() const;
    int constructiveCount() const;
    qreal avgQuality() const;
    QMap<QString, int> categoryCounts() const;
signals:
    void reviewCompleted(int id, qreal quality);
private slots:
    void onReview();
    void onClear();
protected:
    void paintEvent(QPaintEvent*) override;
private:
    void setupUI();
    void drawReviewView(QPainter& p, const QRect& rect);
    void drawCategoryChart(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();
    QList<ReviewExchange2Entry> entries_;
    QSettings settings_;
    QPushButton* reviewBtn_;
    QPushButton* clearBtn_;
    QComboBox* categoryCombo_;
    QLineEdit* inputField_;
    QLabel* infoLabel_;
};
