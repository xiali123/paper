#pragma once
#include <QWidget>
#include <QSettings>
#include <QPainter>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>
struct ProtocolEntry {
    int id; QString name; QString category; QString status;
    int steps; qreal completion; QString version; bool approved; QColor color;
};
class PaperProtocolManager : public QWidget {
    Q_OBJECT
public:
    explicit PaperProtocolManager(QWidget* parent = nullptr);
    void addEntry(const ProtocolEntry& entry);
    QList<ProtocolEntry> entries() const;
    int approvedCount() const;
    qreal avgCompletion() const;
    QMap<QString, int> categoryCounts() const;
signals:
    void protocolUpdated(int id, qreal completion);
private slots:
    void onUpdate();
    void onClear();
protected:
    void paintEvent(QPaintEvent*) override;
private:
    void setupUI();
    void drawProtocolList(QPainter& p, const QRect& rect);
    void drawCategoryChart(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();
    QList<ProtocolEntry> entries_;
    QSettings settings_;
    QPushButton* updateBtn_;
    QPushButton* clearBtn_;
    QComboBox* categoryCombo_;
    QLineEdit* inputField_;
    QLabel* infoLabel_;
};
