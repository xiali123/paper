#pragma once
#include <QWidget>
#include <QSettings>
#include <QPainter>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>
struct ChecksumEntry {
    int id; QString filename; QString category; QString algorithm;
    QString hash; int size; QString date; bool verified; QColor color;
};
class PaperChecksumTool : public QWidget {
    Q_OBJECT
public:
    explicit PaperChecksumTool(QWidget* parent = nullptr);
    void addEntry(const ChecksumEntry& entry);
    QList<ChecksumEntry> entries() const;
    int verifiedCount() const;
    int totalSize() const;
    QMap<QString, int> categoryCounts() const;
signals:
    void checksumVerified(int id, int size);
private slots:
    void onVerify();
    void onClear();
protected:
    void paintEvent(QPaintEvent*) override;
private:
    void setupUI();
    void drawChecksumList(QPainter& p, const QRect& rect);
    void drawCategoryChart(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();
    QList<ChecksumEntry> entries_;
    QSettings settings_;
    QPushButton* verifyBtn_;
    QPushButton* clearBtn_;
    QComboBox* categoryCombo_;
    QLineEdit* inputField_;
    QLabel* infoLabel_;
};
