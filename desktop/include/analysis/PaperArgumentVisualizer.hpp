#pragma once
#include <QWidget>
#include <QLineEdit>
#include <QPushButton>
#include <QComboBox>
#include <QLabel>
#include <QSettings>
#include <QVector>

struct ArgumentNode {
    int id;
    QString claim;
    QString evidence;
    QString type; // premise, conclusion, rebuttal, warrant
    qreal strength;
    int depth;
    bool supported;
    QColor color;
};

class PaperArgumentVisualizer : public QWidget {
    Q_OBJECT
public:
    explicit PaperArgumentVisualizer(QWidget* parent = nullptr);
    void addEntry(const ArgumentNode& entry);
    QList<ArgumentNode> entries() const;
    qreal avgStrength() const;
    int supportedCount() const;
    QMap<QString, int> typeCounts() const;

signals:
    void argumentVisualized(int id, qreal strength);

private slots:
    void onVisualize();
    void onClear();

protected:
    void paintEvent(QPaintEvent*) override;

private:
    void setupUI();
    void drawArgumentTree(QPainter& p, const QRect& rect);
    void drawTypeChart(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();

    QLineEdit* inputField_;
    QPushButton* visualizeBtn_;
    QPushButton* clearBtn_;
    QComboBox* typeCombo_;
    QLabel* infoLabel_;
    QSettings settings_;
    QList<ArgumentNode> entries_;
};
