#pragma once
#include <QWidget>
#include <QSettings>
#include <QPainter>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>
struct JoyPlot2Entry {
    int id; QString distribution; QString category; QString variable;
    qreal density; int samples; bool skewed; QColor color;
};
class PaperJoyPlot2 : public QWidget {
    Q_OBJECT
public:
    explicit PaperJoyPlot2(QWidget* parent = nullptr);
    void addEntry(const JoyPlot2Entry& entry);
    QList<JoyPlot2Entry> entries() const;
    int skewedCount() const;
    qreal avgDensity() const;
    QMap<QString, int> categoryCounts() const;
signals:
    void distributionSelected(int id, qreal density);
private slots:
    void onRender();
    void onClear();
protected:
    void paintEvent(QPaintEvent*) override;
private:
    void setupUI();
    void drawJoyPlot(QPainter& p, const QRect& rect);
    void drawCategoryLegend(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();
    QList<JoyPlot2Entry> entries_;
    QSettings settings_;
    QPushButton* renderBtn_;
    QPushButton* clearBtn_;
    QComboBox* categoryCombo_;
    QLineEdit* inputField_;
    QLabel* infoLabel_;
};
