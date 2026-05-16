#pragma once
#include <QWidget>
#include <QSettings>
#include <QPainter>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>
struct TokenEntry {
    int id; QString token; QString category; QString type;
    qreal frequency; int occurrences; bool keyword; QColor color;
};
class PaperTokenParser : public QWidget {
    Q_OBJECT
public:
    explicit PaperTokenParser(QWidget* parent = nullptr);
    void addEntry(const TokenEntry& entry);
    QList<TokenEntry> entries() const;
    int keywordCount() const;
    qreal avgFrequency() const;
    QMap<QString, int> categoryCounts() const;
signals:
    void tokenFound(int id, qreal frequency);
private slots:
    void onParse();
    void onClear();
protected:
    void paintEvent(QPaintEvent*) override;
private:
    void setupUI();
    void drawTokenView(QPainter& p, const QRect& rect);
    void drawCategoryChart(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();
    QList<TokenEntry> entries_;
    QSettings settings_;
    QPushButton* parseBtn_;
    QPushButton* clearBtn_;
    QComboBox* categoryCombo_;
    QLineEdit* inputField_;
    QLabel* infoLabel_;
};
