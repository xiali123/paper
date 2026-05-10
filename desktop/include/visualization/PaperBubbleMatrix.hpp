#pragma once
#include <QWidget>
#include <QSettings>
#include <QPainter>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>

struct BubbleEntry {
    int id;
    QString row;
    QString col;
    QString category;
    qreal size;
    qreal intensity;
    bool highlighted;
    QColor color;
};

class PaperBubbleMatrix : public QWidget {
    Q_OBJECT
public:
    explicit PaperBubbleMatrix(QWidget* parent = nullptr);
    void addEntry(const BubbleEntry& entry);
    QList<BubbleEntry> entries() const;
    qreal totalSize() const;
    int highlightedCount() const;
    QMap<QString, int> categoryCounts() const;

signals:
    void bubbleCreated(int id, qreal size);

private slots:
    void onGenerate();
    void onClear();

protected:
    void paintEvent(QPaintEvent*) override;

private:
    void setupUI();
    void drawMatrixView(QPainter& p, const QRect& rect);
    void drawCategoryLegend(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();

    QList<BubbleEntry> entries_;
    QSettings settings_;
    QPushButton* generateBtn_;
    QPushButton* clearBtn_;
    QComboBox* categoryCombo_;
    QLineEdit* inputField_;
    QLabel* infoLabel_;
};
