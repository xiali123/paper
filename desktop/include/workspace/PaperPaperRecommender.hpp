#pragma once
#include <QWidget>
#include <QSettings>
#include <QPainter>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>

struct RecommendEntry {
    int id;
    QString title;
    QString author;
    QString category;
    qreal relevance;
    int citations;
    QString reason;
    bool saved;
    QColor color;
};

class PaperPaperRecommender : public QWidget {
    Q_OBJECT
public:
    explicit PaperPaperRecommender(QWidget* parent = nullptr);
    void addEntry(const RecommendEntry& entry);
    QList<RecommendEntry> entries() const;
    int savedCount() const;
    qreal avgRelevance() const;
    QMap<QString, int> categoryCounts() const;

signals:
    void paperRecommended(int id, qreal relevance);

private slots:
    void onRecommend();
    void onClear();

protected:
    void paintEvent(QPaintEvent*) override;

private:
    void setupUI();
    void drawRecommendList(QPainter& p, const QRect& rect);
    void drawCategoryChart(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();

    QList<RecommendEntry> entries_;
    QSettings settings_;
    QPushButton* recommendBtn_;
    QPushButton* clearBtn_;
    QComboBox* categoryCombo_;
    QLineEdit* inputField_;
    QLabel* infoLabel_;
};
