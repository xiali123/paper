#pragma once
#include <QWidget>
#include <QSettings>
#include <QPainter>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>
struct CitationDiffEntry {
    int id; QString paper; QString category; QString change;
    qreal magnitude; int citations; bool added; QColor color;
};
class PaperCitationDiff : public QWidget {
    Q_OBJECT
public:
    explicit PaperCitationDiff(QWidget* parent = nullptr);
    void addEntry(const CitationDiffEntry& entry);
    QList<CitationDiffEntry> entries() const;
    int addedCount() const;
    qreal avgMagnitude() const;
    QMap<QString, int> categoryCounts() const;
signals:
    void diffFound(int id, qreal magnitude);
private slots:
    void onDiff();
    void onClear();
protected:
    void paintEvent(QPaintEvent*) override;
private:
    void setupUI();
    void drawDiffList(QPainter& p, const QRect& rect);
    void drawCategoryChart(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();
    QList<CitationDiffEntry> entries_;
    QSettings settings_;
    QPushButton* diffBtn_;
    QPushButton* clearBtn_;
    QComboBox* categoryCombo_;
    QLineEdit* inputField_;
    QLabel* infoLabel_;
};
