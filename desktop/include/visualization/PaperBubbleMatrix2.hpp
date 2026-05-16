#pragma once
#include <QWidget>
#include <QSettings>
#include <QPainter>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>
struct BubbleMatrix2Entry {
    int id; QString row; QString category; QString column;
    qreal value; int size; bool highlight; QColor color;
};
class PaperBubbleMatrix2 : public QWidget {
    Q_OBJECT
public:
    explicit PaperBubbleMatrix2(QWidget* parent = nullptr);
    void addEntry(const BubbleMatrix2Entry& entry);
    QList<BubbleMatrix2Entry> entries() const;
    int highlightCount() const;
    qreal avgValue() const;
    QMap<QString, int> categoryCounts() const;
signals:
    void bubbleClicked(int id, qreal value);
private slots:
    void onRender();
    void onClear();
protected:
    void paintEvent(QPaintEvent*) override;
private:
    void setupUI();
    void drawBubbleMatrix(QPainter& p, const QRect& rect);
    void drawCategoryLegend(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();
    QList<BubbleMatrix2Entry> entries_;
    QSettings settings_;
    QPushButton* renderBtn_;
    QPushButton* clearBtn_;
    QComboBox* categoryCombo_;
    QLineEdit* inputField_;
    QLabel* infoLabel_;
};
