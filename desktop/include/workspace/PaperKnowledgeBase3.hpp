#pragma once
#include <QWidget>
#include <QSettings>
#include <QPainter>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>
struct KnowledgeBase3Entry {
    int id; QString topic; QString category; QString article;
    qreal coverage; int references; bool complete; QColor color;
};
class PaperKnowledgeBase3 : public QWidget {
    Q_OBJECT
public:
    explicit PaperKnowledgeBase3(QWidget* parent = nullptr);
    void addEntry(const KnowledgeBase3Entry& entry);
    QList<KnowledgeBase3Entry> entries() const;
    int completeCount() const;
    qreal avgCoverage() const;
    QMap<QString, int> categoryCounts() const;
signals:
    void topicUpdated(int id, qreal coverage);
private slots:
    void onUpdate();
    void onClear();
protected:
    void paintEvent(QPaintEvent*) override;
private:
    void setupUI();
    void drawKnowledgeView(QPainter& p, const QRect& rect);
    void drawCategoryChart(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();
    QList<KnowledgeBase3Entry> entries_;
    QSettings settings_;
    QPushButton* updateBtn_;
    QPushButton* clearBtn_;
    QComboBox* categoryCombo_;
    QLineEdit* inputField_;
    QLabel* infoLabel_;
};
