#pragma once
#include <QWidget>
#include <QSettings>
#include <QPainter>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>

struct RingSegment {
    int id;
    QString label;
    QString category;
    qreal value;
    qreal target;
    qreal angle;
    bool complete;
    QColor color;
};

class PaperRingChart : public QWidget {
    Q_OBJECT
public:
    explicit PaperRingChart(QWidget* parent = nullptr);
    void addEntry(const RingSegment& entry);
    QList<RingSegment> entries() const;
    int completeCount() const;
    qreal totalValue() const;
    QMap<QString, int> categoryCounts() const;

signals:
    void ringGenerated(int id, qreal value);

private slots:
    void onGenerate();
    void onClear();

protected:
    void paintEvent(QPaintEvent*) override;

private:
    void setupUI();
    void drawRingView(QPainter& p, const QRect& rect);
    void drawCategoryLegend(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();

    QList<RingSegment> entries_;
    QSettings settings_;
    QPushButton* generateBtn_;
    QPushButton* clearBtn_;
    QComboBox* categoryCombo_;
    QLineEdit* inputField_;
    QLabel* infoLabel_;
};
