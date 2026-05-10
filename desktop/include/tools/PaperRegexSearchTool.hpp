#pragma once
#include <QWidget>
#include <QSettings>
#include <QPushButton>
#include <QLabel>
#include <QLineEdit>
#include <QComboBox>

struct RegexEntry {
    int id;
    QString pattern;
    int matchCount;
    QString source;
    QString flags;
    QString category;
    qreal matchTime;
    QString sampleMatch;
    QColor color;
};

class PaperRegexSearchTool : public QWidget {
    Q_OBJECT
public:
    explicit PaperRegexSearchTool(QWidget* parent = nullptr);
    void addEntry(const RegexEntry& entry);
    QList<RegexEntry> entries() const;
    int totalMatches() const;
    qreal avgMatchTime() const;
    QMap<QString, int> categoryCounts() const;

signals:
    void searchComplete(int id, int matchCount);

protected:
    void paintEvent(QPaintEvent*) override;

private:
    void setupUI();
    void onSearch();
    void onClear();
    void drawResultList(QPainter& p, const QRect& rect);
    void drawCategoryChart(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();

    QSettings settings_;
    QList<RegexEntry> entries_;
    QPushButton* searchBtn_;
    QPushButton* clearBtn_;
    QLineEdit* patternField_;
    QComboBox* categoryCombo_;
    QLabel* infoLabel_;
};
