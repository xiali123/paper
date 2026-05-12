#pragma once
#include <QWidget>
#include <QSettings>
#include <QPainter>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>
struct PipelineEntry {
    int id; QString paper; QString category; QString stage;
    qreal progress; int reviews; bool published; QColor color;
};
class PaperPublicationPipeline : public QWidget {
    Q_OBJECT
public:
    explicit PaperPublicationPipeline(QWidget* parent = nullptr);
    void addEntry(const PipelineEntry& entry);
    QList<PipelineEntry> entries() const;
    int publishedCount() const;
    qreal avgProgress() const;
    QMap<QString, int> categoryCounts() const;
signals:
    void stageAdvanced(int id, qreal progress);
private slots:
    void onAdvance();
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
    QList<PipelineEntry> entries_;
    QSettings settings_;
    QPushButton* advanceBtn_;
    QPushButton* clearBtn_;
    QComboBox* categoryCombo_;
    QLineEdit* inputField_;
    QLabel* infoLabel_;
};
