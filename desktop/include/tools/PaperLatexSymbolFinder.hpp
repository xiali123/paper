#pragma once
#include <QWidget>
#include <QSettings>
#include <QPushButton>
#include <QLabel>
#include <QLineEdit>
#include <QComboBox>

struct SymbolEntry {
    int id;
    QString name;
    QString latexCode;
    QString category;
    qreal relevance;
    int usageFrequency;
    QString description;
    QString unicode;
    QColor color;
};

class PaperLatexSymbolFinder : public QWidget {
    Q_OBJECT
public:
    explicit PaperLatexSymbolFinder(QWidget* parent = nullptr);
    void addSymbol(const SymbolEntry& entry);
    QList<SymbolEntry> symbols() const;
    QMap<QString, int> categoryCounts() const;
    qreal avgRelevance() const;
    int totalUsage() const;

signals:
    void symbolFound(int id, const QString& latexCode);

protected:
    void paintEvent(QPaintEvent*) override;

private:
    void setupUI();
    void onSearch();
    void onClear();
    void drawSymbolList(QPainter& p, const QRect& rect);
    void drawCategoryChart(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();

    QSettings settings_;
    QList<SymbolEntry> symbols_;
    QPushButton* searchBtn_;
    QPushButton* clearBtn_;
    QLineEdit* inputField_;
    QComboBox* categoryCombo_;
    QLabel* infoLabel_;
};
