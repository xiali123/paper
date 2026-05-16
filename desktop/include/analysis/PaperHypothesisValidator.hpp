#pragma once
#include <QWidget>
#include <QSettings>
#include <QPainter>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>
struct HypothesisEntry {
    int id; QString hypothesis; QString category; QString status;
    qreal confidence; int tests; bool validated; QColor color;
};
class PaperHypothesisValidator : public QWidget {
    Q_OBJECT
public:
    explicit PaperHypothesisValidator(QWidget* parent = nullptr);
    void addEntry(const HypothesisEntry& entry);
    QList<HypothesisEntry> entries() const;
    int validatedCount() const;
    qreal avgConfidence() const;
    QMap<QString, int> categoryCounts() const;
signals:
    void hypothesisValidated(int id, qreal confidence);
private slots:
    void onValidate();
    void onClear();
protected:
    void paintEvent(QPaintEvent*) override;
private:
    void setupUI();
    void drawHypothesisView(QPainter& p, const QRect& rect);
    void drawCategoryChart(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();
    QList<HypothesisEntry> entries_;
    QSettings settings_;
    QPushButton* validateBtn_;
    QPushButton* clearBtn_;
    QComboBox* categoryCombo_;
    QLineEdit* inputField_;
    QLabel* infoLabel_;
};
