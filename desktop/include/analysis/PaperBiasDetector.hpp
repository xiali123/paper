#pragma once

#include <QWidget>
#include <QPainter>
#include <QLabel>
#include <QPushButton>
#include <QLineEdit>
#include <QList>
#include <QMap>
#include <QSettings>

struct BiasEntry {
    int id{-1};
    QString text;
    QString biasType; // "confirmation", "selection", "reporting", "anchoring", "availability"
    qreal severity{0};
    QString suggestion;
    bool resolved{false};
    QColor color;
};

class PaperBiasDetector : public QWidget {
    Q_OBJECT

public:
    explicit PaperBiasDetector(QWidget* parent = nullptr);

    void addBias(const BiasEntry& entry);
    QList<BiasEntry> biases() const;
    QMap<QString, int> typeCounts() const;
    qreal avgSeverity() const;
    int unresolvedCount() const;

signals:
    void biasDetected(int id, const QString& type);
    void scanComplete(int total, int critical);

private slots:
    void onAdd();
    void onScan();
    void onResolveAll();
    void onClear();

private:
    void setupUI();
    void paintEvent(QPaintEvent* event) override;
    void drawBiasList(QPainter& p, const QRect& rect);
    void drawSeverityChart(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();

    QLineEdit* inputField_{nullptr};
    QPushButton* addBtn_{nullptr};
    QPushButton* scanBtn_{nullptr};
    QPushButton* resolveBtn_{nullptr};
    QPushButton* clearBtn_{nullptr};
    QLabel* infoLabel_{nullptr};

    QList<BiasEntry> biases_;
    QSettings settings_;
};
