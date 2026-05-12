#pragma once
#include <QWidget>
#include <QSettings>
#include <QPainter>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>
struct LabRotation2Entry {
    int id; QString project; QString category; QString researcher;
    qreal progress; int weeks; bool completed; QColor color;
};
class PaperLabRotation2 : public QWidget {
    Q_OBJECT
public:
    explicit PaperLabRotation2(QWidget* parent = nullptr);
    void addEntry(const LabRotation2Entry& entry);
    QList<LabRotation2Entry> entries() const;
    int completedCount() const;
    qreal avgProgress() const;
    QMap<QString, int> categoryCounts() const;
signals:
    void rotationComplete(int id, qreal progress);
private slots:
    void onRotate();
    void onClear();
protected:
    void paintEvent(QPaintEvent*) override;
private:
    void setupUI();
    void drawRotationView(QPainter& p, const QRect& rect);
    void drawCategoryChart(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();
    QList<LabRotation2Entry> entries_;
    QSettings settings_;
    QPushButton* rotateBtn_;
    QPushButton* clearBtn_;
    QComboBox* categoryCombo_;
    QLineEdit* inputField_;
    QLabel* infoLabel_;
};
