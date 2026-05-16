#pragma once
#include <QWidget>
#include <QSettings>
#include <QPainter>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>
struct StripEntry {
    int id; QString label; QString category; QString group;
    qreal position; qreal width; bool highlight; QColor color;
};
class PaperStripPlot : public QWidget {
    Q_OBJECT
public:
    explicit PaperStripPlot(QWidget* parent = nullptr);
    void addEntry(const StripEntry& entry);
    QList<StripEntry> entries() const;
    int highlightCount() const;
    qreal maxPosition() const;
    QMap<QString, int> categoryCounts() const;
signals:
    void stripSelected(int id, qreal position);
private slots:
    void onRender();
    void onClear();
protected:
    void paintEvent(QPaintEvent*) override;
private:
    void setupUI();
    void drawStripChart(QPainter& p, const QRect& rect);
    void drawCategoryLegend(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();
    QList<StripEntry> entries_;
    QSettings settings_;
    QPushButton* renderBtn_;
    QPushButton* clearBtn_;
    QComboBox* categoryCombo_;
    QLineEdit* inputField_;
    QLabel* infoLabel_;
};
