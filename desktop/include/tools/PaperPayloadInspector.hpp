#pragma once
#include <QWidget>
#include <QSettings>
#include <QPainter>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>
struct PayloadEntry {
    int id; QString endpoint; QString category; QString method;
    qreal size; int fields; bool valid; QColor color;
};
class PaperPayloadInspector : public QWidget {
    Q_OBJECT
public:
    explicit PaperPayloadInspector(QWidget* parent = nullptr);
    void addEntry(const PayloadEntry& entry);
    QList<PayloadEntry> entries() const;
    int validCount() const;
    qreal avgSize() const;
    QMap<QString, int> categoryCounts() const;
signals:
    void payloadChecked(int id, qreal size);
private slots:
    void onInspect();
    void onClear();
protected:
    void paintEvent(QPaintEvent*) override;
private:
    void setupUI();
    void drawPayloadView(QPainter& p, const QRect& rect);
    void drawCategoryChart(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();
    QList<PayloadEntry> entries_;
    QSettings settings_;
    QPushButton* inspectBtn_;
    QPushButton* clearBtn_;
    QComboBox* categoryCombo_;
    QLineEdit* inputField_;
    QLabel* infoLabel_;
};
