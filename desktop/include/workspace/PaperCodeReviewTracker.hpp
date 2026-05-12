#pragma once
#include <QWidget>
#include <QSettings>
#include <QPainter>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>
struct CodeReviewEntry {
    int id; QString title; QString category; QString reviewer;
    qreal complexity; int comments; bool approved; QColor color;
};
class PaperCodeReviewTracker : public QWidget {
    Q_OBJECT
public:
    explicit PaperCodeReviewTracker(QWidget* parent = nullptr);
    void addEntry(const CodeReviewEntry& entry);
    QList<CodeReviewEntry> entries() const;
    int approvedCount() const;
    qreal avgComplexity() const;
    QMap<QString, int> categoryCounts() const;
signals:
    void reviewComplete(int id, qreal complexity);
private slots:
    void onAdd();
    void onClear();
protected:
    void paintEvent(QPaintEvent*) override;
private:
    void setupUI();
    void drawReviewList(QPainter& p, const QRect& rect);
    void drawCategoryChart(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();
    QList<CodeReviewEntry> entries_;
    QSettings settings_;
    QPushButton* addBtn_;
    QPushButton* clearBtn_;
    QComboBox* categoryCombo_;
    QLineEdit* inputField_;
    QLabel* infoLabel_;
};
