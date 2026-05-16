#pragma once
#include <QWidget>
#include <QSettings>
#include <QPainter>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>

struct BiasEntry {
    int id;
    QString section;
    QString biasType;
    QString category;
    qreal severity;
    qreal confidence;
    bool flagged;
    QColor color;
};

class PaperBiasScanner : public QWidget {
    Q_OBJECT
public:
    explicit PaperBiasScanner(QWidget* parent = nullptr);
    void addEntry(const BiasEntry& entry);
    QList<BiasEntry> entries() const;
    int flaggedCount() const;
    qreal avgSeverity() const;
    QMap<QString, int> categoryCounts() const;

signals:
    void biasDetected(int id, qreal severity);

private slots:
    void onScan();
    void onClear();

protected:
    void paintEvent(QPaintEvent*) override;

private:
    void setupUI();
    void drawBiasList(QPainter& p, const QRect& rect);
    void drawCategoryChart(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();

    QList<BiasEntry> entries_;
    QSettings settings_;
    QPushButton* scanBtn_;
    QPushButton* clearBtn_;
    QComboBox* categoryCombo_;
    QLineEdit* inputField_;
    QLabel* infoLabel_;
};
