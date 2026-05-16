#pragma once
#include <QWidget>
#include <QLineEdit>
#include <QPushButton>
#include <QComboBox>
#include <QLabel>
#include <QSettings>
#include <QVector>

struct ArgumentMine {
    int id;
    QString premise;
    QString conclusion;
    QString relation;
    qreal confidence;
    int support;
    QString source;
    bool valid;
    QColor color;
};

class PaperArgumentMiner : public QWidget {
    Q_OBJECT
public:
    explicit PaperArgumentMiner(QWidget* parent = nullptr);
    void addEntry(const ArgumentMine& entry);
    QList<ArgumentMine> entries() const;
    int validCount() const;
    qreal avgConfidence() const;
    QMap<QString, int> relationCounts() const;

signals:
    void argumentMined(int id, qreal confidence);

private slots:
    void onMine();
    void onClear();

protected:
    void paintEvent(QPaintEvent*) override;

private:
    void setupUI();
    void drawArgumentList(QPainter& p, const QRect& rect);
    void drawRelationChart(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();

    QLineEdit* inputField_;
    QPushButton* mineBtn_;
    QPushButton* clearBtn_;
    QComboBox* relationCombo_;
    QLabel* infoLabel_;
    QSettings settings_;
    QList<ArgumentMine> entries_;
};
