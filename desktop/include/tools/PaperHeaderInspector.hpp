#pragma once
#include <QWidget>
#include <QSettings>
#include <QPainter>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>
struct HeaderInspectorEntry {
    int id; QString url; QString category; QString header;
    qreal score; int checks; bool secure; QColor color;
};
class PaperHeaderInspector : public QWidget {
    Q_OBJECT
public:
    explicit PaperHeaderInspector(QWidget* parent = nullptr);
    void addEntry(const HeaderInspectorEntry& entry);
    QList<HeaderInspectorEntry> entries() const;
    int secureCount() const;
    qreal avgScore() const;
    QMap<QString, int> categoryCounts() const;
signals:
    void headerInspected(int id, qreal score);
private slots:
    void onInspect();
    void onClear();
protected:
    void paintEvent(QPaintEvent*) override;
private:
    void setupUI();
    void drawInspectorView(QPainter& p, const QRect& rect);
    void drawCategoryChart(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();
    QList<HeaderInspectorEntry> entries_;
    QSettings settings_;
    QPushButton* inspectBtn_;
    QPushButton* clearBtn_;
    QComboBox* categoryCombo_;
    QLineEdit* inputField_;
    QLabel* infoLabel_;
};
