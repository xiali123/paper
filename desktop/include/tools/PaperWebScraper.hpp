#pragma once
#include <QWidget>
#include <QList>
#include <QMap>
#include <QSettings>

struct ScraperEntry {
    int id;
    QString url;
    QString selector;
    int papersFound;
    qreal relevance;
    QString status;
    QString source;
    int pagesScanned;
    qreal confidence;
    bool successful;
    QColor color;
};

class PaperWebScraper : public QWidget {
    Q_OBJECT
public:
    explicit PaperWebScraper(QWidget* parent = nullptr);
    void addEntry(const ScraperEntry& entry);
    QList<ScraperEntry> entries() const;
    int successfulCount() const;
    qreal avgRelevance() const;
    QMap<QString, int> sourceCounts() const;
signals:
    void scrapeCompleted(int id, qreal relevance);
private slots:
    void onScrape();
    void onClear();
private:
    void setupUI();
    void paintEvent(QPaintEvent*) override;
    void drawScraperList(QPainter& p, const QRect& rect);
    void drawSourceChart(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();
    QList<ScraperEntry> entries_;
    QSettings settings_;
    QPushButton* scrapeBtn_;
    QPushButton* clearBtn_;
    QComboBox* sourceCombo_;
    QLineEdit* inputField_;
    QLabel* infoLabel_;
};
