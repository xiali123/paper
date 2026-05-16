#pragma once
#include <QWidget>
#include <QSettings>
#include <QPainter>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>
struct KBEntry {
    int id; QString title; QString category; QString section;
    qreal completeness; int views; bool verified; QColor color;
};
class PaperKnowledgeBase2 : public QWidget {
    Q_OBJECT
public:
    explicit PaperKnowledgeBase2(QWidget* parent = nullptr);
    void addEntry(const KBEntry& entry);
    QList<KBEntry> entries() const;
    int verifiedCount() const;
    qreal avgCompleteness() const;
    QMap<QString, int> categoryCounts() const;
signals:
    void articleSaved(int id, qreal completeness);
private slots:
    void onSave();
    void onClear();
protected:
    void paintEvent(QPaintEvent*) override;
private:
    void setupUI();
    void drawArticleList(QPainter& p, const QRect& rect);
    void drawCategoryChart(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();
    QList<KBEntry> entries_;
    QSettings settings_;
    QPushButton* saveBtn_;
    QPushButton* clearBtn_;
    QComboBox* categoryCombo_;
    QLineEdit* inputField_;
    QLabel* infoLabel_;
};
