#pragma once
#include <QWidget>
#include <QSettings>
#include <QPainter>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>
struct CompassEntry {
    int id; QString paper; QString category; QString direction;
    qreal relevance; qreal depth; qreal breadth; bool onCourse; QColor color;
};
class PaperReadingCompass : public QWidget {
    Q_OBJECT
public:
    explicit PaperReadingCompass(QWidget* parent = nullptr);
    void addEntry(const CompassEntry& entry);
    QList<CompassEntry> entries() const;
    int onCourseCount() const;
    qreal avgDepth() const;
    QMap<QString, int> categoryCounts() const;
signals:
    void compassUpdated(int id, qreal relevance);
private slots:
    void onNavigate();
    void onClear();
protected:
    void paintEvent(QPaintEvent*) override;
private:
    void setupUI();
    void drawCompassView(QPainter& p, const QRect& rect);
    void drawCategoryChart(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();
    QList<CompassEntry> entries_;
    QSettings settings_;
    QPushButton* navigateBtn_;
    QPushButton* clearBtn_;
    QComboBox* categoryCombo_;
    QLineEdit* inputField_;
    QLabel* infoLabel_;
};
