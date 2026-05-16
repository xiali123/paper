#pragma once
#include <QWidget>
#include <QSettings>
#include <QPainter>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>
struct BiasDetector3Entry {
    int id; QString text; QString category; QString bias;
    qreal severity; int instances; bool flagged; QColor color;
};
class PaperBiasDetector3 : public QWidget {
    Q_OBJECT
public:
    explicit PaperBiasDetector3(QWidget* parent = nullptr);
    void addEntry(const BiasDetector3Entry& entry);
    QList<BiasDetector3Entry> entries() const;
    int flaggedCount() const;
    qreal avgSeverity() const;
    QMap<QString, int> categoryCounts() const;
signals:
    void biasDetected(int id, qreal severity);
private slots:
    void onDetect();
    void onClear();
protected:
    void paintEvent(QPaintEvent*) override;
private:
    void setupUI();
    void drawBiasView(QPainter& p, const QRect& rect);
    void drawCategoryChart(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();
    QList<BiasDetector3Entry> entries_;
    QSettings settings_;
    QPushButton* detectBtn_;
    QPushButton* clearBtn_;
    QComboBox* categoryCombo_;
    QLineEdit* inputField_;
    QLabel* infoLabel_;
};
