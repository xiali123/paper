#pragma once
#include <QWidget>
#include <QSettings>
#include <QPainter>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>
struct ReviewPipelineEntry {
    int id; QString paper; QString category; QString stage;
    qreal score; int reviewers; bool accepted; QColor color;
};
class PaperReviewPipeline : public QWidget {
    Q_OBJECT
public:
    explicit PaperReviewPipeline(QWidget* parent = nullptr);
    void addEntry(const ReviewPipelineEntry& entry);
    QList<ReviewPipelineEntry> entries() const;
    int acceptedCount() const;
    qreal avgScore() const;
    QMap<QString, int> categoryCounts() const;
signals:
    void reviewUpdated(int id, qreal score);
private slots:
    void onTrack();
    void onClear();
protected:
    void paintEvent(QPaintEvent*) override;
private:
    void setupUI();
    void drawPipelineView(QPainter& p, const QRect& rect);
    void drawCategoryChart(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();
    QList<ReviewPipelineEntry> entries_;
    QSettings settings_;
    QPushButton* trackBtn_;
    QPushButton* clearBtn_;
    QComboBox* categoryCombo_;
    QLineEdit* inputField_;
    QLabel* infoLabel_;
};
