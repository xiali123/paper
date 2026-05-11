#pragma once
#include <QWidget>
#include <QSettings>
#include <QPainter>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>

struct NoveltyEntry {
    int id;
    QString method;
    QString metric;
    QString category;
    qreal novelty;
    qreal baseline;
    qreal improvement;
    bool novel;
    QColor color;
};

class PaperNoveltyScore : public QWidget {
    Q_OBJECT
public:
    explicit PaperNoveltyScore(QWidget* parent = nullptr);
    void addEntry(const NoveltyEntry& entry);
    QList<NoveltyEntry> entries() const;
    int novelCount() const;
    qreal avgNovelty() const;
    QMap<QString, int> categoryCounts() const;

signals:
    void noveltyScored(int id, qreal novelty);

private slots:
    void onScore();
    void onClear();

protected:
    void paintEvent(QPaintEvent*) override;

private:
    void setupUI();
    void drawNoveltyList(QPainter& p, const QRect& rect);
    void drawCategoryChart(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();

    QList<NoveltyEntry> entries_;
    QSettings settings_;
    QPushButton* scoreBtn_;
    QPushButton* clearBtn_;
    QComboBox* categoryCombo_;
    QLineEdit* inputField_;
    QLabel* infoLabel_;
};
