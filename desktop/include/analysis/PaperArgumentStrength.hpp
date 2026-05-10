#pragma once
#include <QWidget>
#include <QSettings>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>
#include <QVector>

struct ArgumentEntry {
    int id;
    QString claim;
    qreal strength;
    QString evidenceType;
    int sourceCount;
    qreal consistency;
    QString section;
    QString category;
    bool strong;
    QColor color;
};

class PaperArgumentStrength : public QWidget {
    Q_OBJECT
public:
    explicit PaperArgumentStrength(QWidget* parent = nullptr);
    void addEntry(const ArgumentEntry& entry);
    QList<ArgumentEntry> entries() const;
    qreal avgStrength() const;
    int strongCount() const;
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
    void drawArgumentList(QPainter& p, const QRect& rect);
    void drawStrengthChart(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();

    QSettings settings_;
    QVector<ArgumentEntry> entries_;
    QPushButton* evaluateBtn_;
    QPushButton* clearBtn_;
    QComboBox* filterCombo_;
    QLineEdit* inputField_;
    QLabel* infoLabel_;
};
