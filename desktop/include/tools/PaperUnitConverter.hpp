#pragma once
#include <QWidget>
#include <QSettings>
#include <QPainter>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>
struct ConversionEntry {
    int id; QString fromUnit; QString category; QString toUnit;
    qreal inputValue; qreal outputValue; qreal factor; bool favorite; QColor color;
};
class PaperUnitConverter : public QWidget {
    Q_OBJECT
public:
    explicit PaperUnitConverter(QWidget* parent = nullptr);
    void addEntry(const ConversionEntry& entry);
    QList<ConversionEntry> entries() const;
    int favoriteCount() const;
    qreal lastFactor() const;
    QMap<QString, int> categoryCounts() const;
signals:
    void conversionDone(int id, qreal outputValue);
private slots:
    void onConvert();
    void onClear();
protected:
    void paintEvent(QPaintEvent*) override;
private:
    void setupUI();
    void drawConversionList(QPainter& p, const QRect& rect);
    void drawCategoryChart(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();
    QList<ConversionEntry> entries_;
    QSettings settings_;
    QPushButton* convertBtn_;
    QPushButton* clearBtn_;
    QComboBox* categoryCombo_;
    QLineEdit* inputField_;
    QLabel* infoLabel_;
};
