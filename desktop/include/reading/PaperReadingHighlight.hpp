#pragma once
#include <QWidget>
#include <QSettings>
#include <QPainter>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>
struct HighlightEntry {
    int id; QString text; QString category; QString color_name;
    qreal importance; int position; bool starred; QColor color;
};
class PaperReadingHighlight : public QWidget {
    Q_OBJECT
public:
    explicit PaperReadingHighlight(QWidget* parent = nullptr);
    void addEntry(const HighlightEntry& entry);
    QList<HighlightEntry> entries() const;
    int starredCount() const;
    qreal avgImportance() const;
    QMap<QString, int> categoryCounts() const;
signals:
    void highlightAdded(int id, qreal importance);
private slots:
    void onAdd();
    void onClear();
protected:
    void paintEvent(QPaintEvent*) override;
private:
    void setupUI();
    void drawHighlightList(QPainter& p, const QRect& rect);
    void drawCategoryChart(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();
    QList<HighlightEntry> entries_;
    QSettings settings_;
    QPushButton* addBtn_;
    QPushButton* clearBtn_;
    QComboBox* categoryCombo_;
    QLineEdit* inputField_;
    QLabel* infoLabel_;
};
