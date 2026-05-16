#pragma once
#include <QWidget>
#include <QSettings>
#include <QPainter>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>
struct EmbedEntry {
    int id; QString label; QString category; QString model;
    qreal x; qreal y; int dimensions; bool outlier; QColor color;
};
class PaperEmbeddingVisualizer : public QWidget {
    Q_OBJECT
public:
    explicit PaperEmbeddingVisualizer(QWidget* parent = nullptr);
    void addEntry(const EmbedEntry& entry);
    QList<EmbedEntry> entries() const;
    int outlierCount() const;
    qreal avgDistance() const;
    QMap<QString, int> categoryCounts() const;
signals:
    void embeddingProjected(int id, qreal distance);
private slots:
    void onProject();
    void onClear();
protected:
    void paintEvent(QPaintEvent*) override;
private:
    void setupUI();
    void drawEmbedView(QPainter& p, const QRect& rect);
    void drawCategoryChart(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();
    QList<EmbedEntry> entries_;
    QSettings settings_;
    QPushButton* projectBtn_;
    QPushButton* clearBtn_;
    QComboBox* categoryCombo_;
    QLineEdit* inputField_;
    QLabel* infoLabel_;
};
