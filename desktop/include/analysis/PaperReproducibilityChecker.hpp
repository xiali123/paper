#pragma once

#include <QWidget>
#include <QPainter>
#include <QLabel>
#include <QPushButton>
#include <QLineEdit>
#include <QList>
#include <QMap>
#include <QSettings>

struct ReproCheck {
    int id{-1};
    QString criterion;
    QString category; // "data", "code", "method", "documentation"
    bool passed{false};
    qreal importance{0};
    QString notes;
    QColor color;
};

class PaperReproducibilityChecker : public QWidget {
    Q_OBJECT

public:
    explicit PaperReproducibilityChecker(QWidget* parent = nullptr);

    void addCheck(const ReproCheck& check);
    QList<ReproCheck> checks() const;
    QMap<QString, int> categoryCounts() const;
    qreal reproducibilityScore() const;
    int passedCount() const;

signals:
    void checkCompleted(int passed, int total);
    void scoreUpdated(qreal score);

private slots:
    void onAdd();
    void onEvaluate();
    void onClear();

private:
    void setupUI();
    void paintEvent(QPaintEvent* event) override;
    void drawCheckList(QPainter& p, const QRect& rect);
    void drawScoreGauge(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();

    QLineEdit* inputField_{nullptr};
    QPushButton* addBtn_{nullptr};
    QPushButton* evaluateBtn_{nullptr};
    QPushButton* clearBtn_{nullptr};
    QLabel* infoLabel_{nullptr};

    QList<ReproCheck> checks_;
    QSettings settings_;
};
