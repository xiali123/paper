#pragma once
#include <QWidget>
#include <QSettings>
#include <QPainter>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>
struct PublicationPipeline2Entry {
    int id; QString paper; QString category; QString stage;
    qreal progress; int reviews; bool accepted; QColor color;
};
class PaperPublicationPipeline2 : public QWidget {
    Q_OBJECT
public:
    explicit PaperPublicationPipeline2(QWidget* parent = nullptr);
    void addEntry(const PublicationPipeline2Entry& entry);
    QList<PublicationPipeline2Entry> entries() const;
    int acceptedCount() const;
    qreal avgProgress() const;
    QMap<QString, int> categoryCounts() const;
signals:
    void stageChanged(int id, qreal progress);
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
    QList<PublicationPipeline2Entry> entries_;
    QSettings settings_;
    QPushButton* trackBtn_;
    QPushButton* clearBtn_;
    QComboBox* categoryCombo_;
    QLineEdit* inputField_;
    QLabel* infoLabel_;
};
