#pragma once
#include <QWidget>
#include <QSettings>
#include <QPainter>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>
struct QuoteVaultEntry {
    int id; QString quote; QString category; QString source;
    qreal relevance; int citations; bool starred; QColor color;
};
class PaperQuoteVault : public QWidget {
    Q_OBJECT
public:
    explicit PaperQuoteVault(QWidget* parent = nullptr);
    void addEntry(const QuoteVaultEntry& entry);
    QList<QuoteVaultEntry> entries() const;
    int starredCount() const;
    qreal avgRelevance() const;
    QMap<QString, int> categoryCounts() const;
signals:
    void quoteSaved(int id, qreal relevance);
private slots:
    void onSave();
    void onClear();
protected:
    void paintEvent(QPaintEvent*) override;
private:
    void setupUI();
    void drawQuoteView(QPainter& p, const QRect& rect);
    void drawCategoryChart(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();
    QList<QuoteVaultEntry> entries_;
    QSettings settings_;
    QPushButton* saveBtn_;
    QPushButton* clearBtn_;
    QComboBox* categoryCombo_;
    QLineEdit* inputField_;
    QLabel* infoLabel_;
};
