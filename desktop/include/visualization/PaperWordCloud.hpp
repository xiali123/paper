#pragma once
#include <QWidget>
#include <QLineEdit>
#include <QPushButton>
#include <QComboBox>
#include <QLabel>
#include <QSettings>
#include <QVector>

struct WordCloudEntry {
    int id;
    QString word;
    int frequency;
    qreal weight;
    QString category;
    int rank;
    qreal tfidf;
    bool highlighted;
    QColor color;
};

class PaperWordCloud : public QWidget {
    Q_OBJECT
public:
    explicit PaperWordCloud(QWidget* parent = nullptr);
    void addEntry(const WordCloudEntry& entry);
    QList<WordCloudEntry> entries() const;
    int totalFrequency() const;
    int highlightedCount() const;
    QMap<QString, int> categoryCounts() const;

signals:
    void wordCloudGenerated(int id, int frequency);

private slots:
    void onGenerate();
    void onClear();

protected:
    void paintEvent(QPaintEvent*) override;

private:
    void setupUI();
    void drawCloudView(QPainter& p, const QRect& rect);
    void drawCategoryLegend(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();

    QLineEdit* inputField_;
    QPushButton* generateBtn_;
    QPushButton* clearBtn_;
    QComboBox* categoryCombo_;
    QLabel* infoLabel_;
    QSettings settings_;
    QList<WordCloudEntry> entries_;
};
