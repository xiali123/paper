#pragma once
#include <QWidget>
#include <QSettings>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>
#include <QVector>

struct BiasEntry {
    int id;
    QString text;
    QString biasType;
    qreal score;
    QString context;
    int occurrence;
    QString suggestion;
    QString category;
    bool flagged;
    QColor color;
};

class PaperBiasChecker : public QWidget {
    Q_OBJECT
public:
    explicit PaperBiasChecker(QWidget* parent = nullptr);
    void addEntry(const BiasEntry& entry);
    QList<BiasEntry> entries() const;
    qreal avgScore() const;
    int flaggedCount() const;
    QMap<QString, int> biasTypeCounts() const;

signals:
    void biasDetected(int id, qreal score);

private slots:
    void onCheck();
    void onClear();

protected:
    void paintEvent(QPaintEvent*) override;

private:
    void setupUI();
    void drawBiasList(QPainter& p, const QRect& rect);
    void drawTypeChart(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();

    QSettings settings_;
    QVector<BiasEntry> entries_;
    QPushButton* checkBtn_;
    QPushButton* clearBtn_;
    QComboBox* filterCombo_;
    QLineEdit* inputField_;
    QLabel* infoLabel_;
};
