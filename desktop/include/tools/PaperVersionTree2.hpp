#pragma once
#include <QWidget>
#include <QSettings>
#include <QPainter>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>
struct VersionTree2Entry {
    int id; QString version; QString category; QString branch;
    qreal size; int commits; bool stable; QColor color;
};
class PaperVersionTree2 : public QWidget {
    Q_OBJECT
public:
    explicit PaperVersionTree2(QWidget* parent = nullptr);
    void addEntry(const VersionTree2Entry& entry);
    QList<VersionTree2Entry> entries() const;
    int stableCount() const;
    qreal avgSize() const;
    QMap<QString, int> categoryCounts() const;
signals:
    void versionSelected(int id, qreal size);
private slots:
    void onRender();
    void onClear();
protected:
    void paintEvent(QPaintEvent*) override;
private:
    void setupUI();
    void drawTreeView(QPainter& p, const QRect& rect);
    void drawCategoryChart(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();
    QList<VersionTree2Entry> entries_;
    QSettings settings_;
    QPushButton* renderBtn_;
    QPushButton* clearBtn_;
    QComboBox* categoryCombo_;
    QLineEdit* inputField_;
    QLabel* infoLabel_;
};
