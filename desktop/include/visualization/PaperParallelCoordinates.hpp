#pragma once
#include <QWidget>
#include <QLineEdit>
#include <QPushButton>
#include <QComboBox>
#include <QLabel>
#include <QSettings>
#include <QVector>

struct ParallelEntry {
    int id;
    QString label;
    qreal dim1;
    qreal dim2;
    qreal dim3;
    qreal dim4;
    QString category;
    qreal aggregate;
    bool outlier;
    QColor color;
};

class PaperParallelCoordinates : public QWidget {
    Q_OBJECT
public:
    explicit PaperParallelCoordinates(QWidget* parent = nullptr);
    void addEntry(const ParallelEntry& entry);
    QList<ParallelEntry> entries() const;
    int outlierCount() const;
    qreal avgAggregate() const;
    QMap<QString, int> categoryCounts() const;

signals:
    void parallelGenerated(int id, qreal aggregate);

private slots:
    void onGenerate();
    void onClear();

protected:
    void paintEvent(QPaintEvent*) override;

private:
    void setupUI();
    void drawParallelView(QPainter& p, const QRect& rect);
    void drawCategoryLegend(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();

    QLineEdit* inputField_;
    QPushButton* generateBtn_;
    QPushButton* clearBtn_;
    QComboBox* categoryCombo_;
    QLabel* infoLabel_;
    QSettings settings_;
    QList<ParallelEntry> entries_;
};
