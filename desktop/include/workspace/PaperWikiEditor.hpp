#pragma once
#include <QWidget>
#include <QSettings>
#include <QPainter>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>
struct WikiEntry {
    int id; QString title; QString category; QString section;
    qreal completeness; int edits; bool reviewed; QColor color;
};
class PaperWikiEditor : public QWidget {
    Q_OBJECT
public:
    explicit PaperWikiEditor(QWidget* parent = nullptr);
    void addEntry(const WikiEntry& entry);
    QList<WikiEntry> entries() const;
    int reviewedCount() const;
    qreal avgCompleteness() const;
    QMap<QString, int> categoryCounts() const;
signals:
    void pageSaved(int id, qreal completeness);
private slots:
    void onSave();
    void onClear();
protected:
    void paintEvent(QPaintEvent*) override;
private:
    void setupUI();
    void drawPageList(QPainter& p, const QRect& rect);
    void drawCategoryChart(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();
    QList<WikiEntry> entries_;
    QSettings settings_;
    QPushButton* saveBtn_;
    QPushButton* clearBtn_;
    QComboBox* categoryCombo_;
    QLineEdit* inputField_;
    QLabel* infoLabel_;
};
