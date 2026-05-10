#pragma once
#include <QWidget>
#include <QLineEdit>
#include <QComboBox>
#include <QPushButton>
#include <QLabel>
#include <QSettings>
#include <QPainter>
#include <QList>

struct TableExtractEntry {
    int id;
    QString paperTitle;
    int tableNum;
    int rows;
    int cols;
    qreal confidence;
    QString format;
    bool hasHeader;
    QString content;
    QColor color;
};

class PaperTableExtractor : public QWidget {
    Q_OBJECT
public:
    explicit PaperTableExtractor(QWidget* parent = nullptr);
    void addEntry(const TableExtractEntry& entry);
    QList<TableExtractEntry> entries() const;
    int totalCells() const;
    qreal avgConfidence() const;
    QMap<QString, int> formatCounts() const;

signals:
    void tableExtracted(int id, int tableNum);

protected:
    void paintEvent(QPaintEvent*) override;

private:
    void setupUI();
    void onExtract();
    void onClear();
    void drawTableList(QPainter& p, const QRect& rect);
    void drawFormatChart(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();

    QLineEdit* inputField_;
    QComboBox* formatCombo_;
    QPushButton* extractBtn_;
    QPushButton* clearBtn_;
    QLabel* infoLabel_;
    QList<TableExtractEntry> entries_;
    QSettings settings_;
};
