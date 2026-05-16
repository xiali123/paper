#pragma once
#include <QWidget>
#include <QSettings>
#include <QPainter>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>

struct RegexEntry {
    int id;
    QString pattern;
    QString category;
    QString testStr;
    int matches;
    bool valid;
    bool caseSensitive;
    QColor color;
};

class PaperRegexTester : public QWidget {
    Q_OBJECT
public:
    explicit PaperRegexTester(QWidget* parent = nullptr);
    void addEntry(const RegexEntry& entry);
    QList<RegexEntry> entries() const;
    int validCount() const;
    int totalMatches() const;
    QMap<QString, int> categoryCounts() const;
signals:
    void regexTested(int id, int matches);
private slots:
    void onTest();
    void onClear();
protected:
    void paintEvent(QPaintEvent*) override;
private:
    void setupUI();
    void drawRegexList(QPainter& p, const QRect& rect);
    void drawCategoryChart(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();
    QList<RegexEntry> entries_;
    QSettings settings_;
    QPushButton* testBtn_;
    QPushButton* clearBtn_;
    QComboBox* categoryCombo_;
    QLineEdit* inputField_;
    QLabel* infoLabel_;
};
