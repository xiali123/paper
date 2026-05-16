#pragma once
#include <QWidget>
#include <QSettings>
#include <QPainter>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>
struct ReviewEntry {
    int id; QString paper; QString category; QString reviewer;
    qreal score; int comments; bool accepted; QColor color;
};
class PaperPeerReview : public QWidget {
    Q_OBJECT
public:
    explicit PaperPeerReview(QWidget* parent = nullptr);
    void addEntry(const ReviewEntry& entry);
    QList<ReviewEntry> entries() const;
    int acceptedCount() const;
    qreal avgScore() const;
    QMap<QString, int> categoryCounts() const;
signals:
    void reviewDone(int id, qreal score);
private slots:
    void onReview();
    void onClear();
protected:
    void paintEvent(QPaintEvent*) override;
private:
    void setupUI();
    void drawReviewBoard(QPainter& p, const QRect& rect);
    void drawCategoryChart(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();
    QList<ReviewEntry> entries_;
    QSettings settings_;
    QPushButton* reviewBtn_;
    QPushButton* clearBtn_;
    QComboBox* categoryCombo_;
    QLineEdit* inputField_;
    QLabel* infoLabel_;
};
