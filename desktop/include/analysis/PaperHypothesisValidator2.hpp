#pragma once
#include <QWidget>
#include <QSettings>
#include <QPainter>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>
struct HypothesisValidator2Entry {
    int id; QString hypothesis; QString category; QString test;
    qreal pvalue; int samples; bool significant; QColor color;
};
class PaperHypothesisValidator2 : public QWidget {
    Q_OBJECT
public:
    explicit PaperHypothesisValidator2(QWidget* parent = nullptr);
    void addEntry(const HypothesisValidator2Entry& entry);
    QList<HypothesisValidator2Entry> entries() const;
    int significantCount() const;
    qreal avgPvalue() const;
    QMap<QString, int> categoryCounts() const;
signals:
    void testComplete(int id, qreal pvalue);
private slots:
    void onValidate();
    void onClear();
protected:
    void paintEvent(QPaintEvent*) override;
private:
    void setupUI();
    void drawValidatorView(QPainter& p, const QRect& rect);
    void drawCategoryChart(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();
    QList<HypothesisValidator2Entry> entries_;
    QSettings settings_;
    QPushButton* validateBtn_;
    QPushButton* clearBtn_;
    QComboBox* categoryCombo_;
    QLineEdit* inputField_;
    QLabel* infoLabel_;
};
