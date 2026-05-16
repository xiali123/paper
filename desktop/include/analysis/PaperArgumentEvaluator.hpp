#pragma once
#include <QWidget>
#include <QSettings>
#include <QPainter>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>
struct ArgumentEvalEntry {
    int id; QString claim; QString category; QString stance;
    qreal strength; int evidence; bool convincing; QColor color;
};
class PaperArgumentEvaluator : public QWidget {
    Q_OBJECT
public:
    explicit PaperArgumentEvaluator(QWidget* parent = nullptr);
    void addEntry(const ArgumentEvalEntry& entry);
    QList<ArgumentEvalEntry> entries() const;
    int convincingCount() const;
    qreal avgStrength() const;
    QMap<QString, int> categoryCounts() const;
signals:
    void argumentEvaluated(int id, qreal strength);
private slots:
    void onEvaluate();
    void onClear();
protected:
    void paintEvent(QPaintEvent*) override;
private:
    void setupUI();
    void drawArgumentMatrix(QPainter& p, const QRect& rect);
    void drawCategoryChart(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();
    QList<ArgumentEvalEntry> entries_;
    QSettings settings_;
    QPushButton* evaluateBtn_;
    QPushButton* clearBtn_;
    QComboBox* categoryCombo_;
    QLineEdit* inputField_;
    QLabel* infoLabel_;
};
