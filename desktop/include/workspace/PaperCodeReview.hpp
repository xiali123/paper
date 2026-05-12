#pragma once
#include <QWidget>
#include <QSettings>
#include <QPainter>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>
struct CodeReviewEntry {
    int id; QString file; QString category; QString reviewer;
    qreal score; int issues; bool approved; QColor color;
};
class PaperCodeReview : public QWidget {
    Q_OBJECT
public:
    explicit PaperCodeReview(QWidget* parent = nullptr);
    void addEntry(const CodeReviewEntry& entry);
    QList<CodeReviewEntry> entries() const;
    int approvedCount() const;
    qreal avgScore() const;
    QMap<QString, int> categoryCounts() const;
signals:
    void reviewCompleted(int id, qreal score);
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
    QList<CodeReviewEntry> entries_;
    QSettings settings_;
    QPushButton* reviewBtn_;
    QPushButton* clearBtn_;
    QComboBox* categoryCombo_;
    QLineEdit* inputField_;
    QLabel* infoLabel_;
};
