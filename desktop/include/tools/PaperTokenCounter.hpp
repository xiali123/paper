#pragma once
#include <QWidget>
#include <QSettings>
#include <QPainter>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>
struct TokenEntry {
    int id; QString text; QString category; QString type;
    int tokens; qreal ratio; int unique; bool truncated; QColor color;
};
class PaperTokenCounter : public QWidget {
    Q_OBJECT
public:
    explicit PaperTokenCounter(QWidget* parent = nullptr);
    void addEntry(const TokenEntry& entry);
    QList<TokenEntry> entries() const;
    int truncatedCount() const;
    qreal avgTokens() const;
    QMap<QString, int> categoryCounts() const;
signals:
    void tokensCounted(int id, int tokens);
private slots:
    void onCount();
    void onClear();
protected:
    void paintEvent(QPaintEvent*) override;
private:
    void setupUI();
    void drawTokenList(QPainter& p, const QRect& rect);
    void drawCategoryChart(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();
    QList<TokenEntry> entries_;
    QSettings settings_;
    QPushButton* countBtn_;
    QPushButton* clearBtn_;
    QComboBox* categoryCombo_;
    QLineEdit* inputField_;
    QLabel* infoLabel_;
};
