#pragma once
#include <QWidget>
#include <QSettings>
#include <QPainter>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>
struct StripPlot2Entry {
    int id; QString series; QString category; QString group;
    qreal value; int position; bool outlier; QColor color;
};
class PaperStripPlot2 : public QWidget {
    Q_OBJECT
public:
    explicit PaperStripPlot2(QWidget* parent = nullptr);
    void addEntry(const StripPlot2Entry& entry);
    QList<StripPlot2Entry> entries() const;
    int outlierCount() const;
    qreal medianValue() const;
    QMap<QString, int> categoryCounts() const;
signals:
    void pointSelected(int id, qreal value);
private slots:
    void onPlot();
    void onClear();
protected:
    void paintEvent(QPaintEvent*) override;
private:
    void setupUI();
    void drawStripPlot(QPainter& p, const QRect& rect);
    void drawCategoryLegend(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();
    QList<StripPlot2Entry> entries_;
    QSettings settings_;
    QPushButton* plotBtn_;
    QPushButton* clearBtn_;
    QComboBox* categoryCombo_;
    QLineEdit* inputField_;
    QLabel* infoLabel_;
};
