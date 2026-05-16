#pragma once
#include <QWidget>
#include <QSettings>
#include <QPainter>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>
struct WikiForge2Entry {
    int id; QString article; QString category; QString editor;
    qreal quality; int edits; bool featured; QColor color;
};
class PaperWikiForge2 : public QWidget {
    Q_OBJECT
public:
    explicit PaperWikiForge2(QWidget* parent = nullptr);
    void addEntry(const WikiForge2Entry& entry);
    QList<WikiForge2Entry> entries() const;
    int featuredCount() const;
    qreal avgQuality() const;
    QMap<QString, int> categoryCounts() const;
signals:
    void articlePublished(int id, qreal quality);
private slots:
    void onEdit();
    void onClear();
protected:
    void paintEvent(QPaintEvent*) override;
private:
    void setupUI();
    void drawForgeView(QPainter& p, const QRect& rect);
    void drawCategoryChart(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();
    QList<WikiForge2Entry> entries_;
    QSettings settings_;
    QPushButton* editBtn_;
    QPushButton* clearBtn_;
    QComboBox* categoryCombo_;
    QLineEdit* inputField_;
    QLabel* infoLabel_;
};
