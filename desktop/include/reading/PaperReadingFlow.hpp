#pragma once
#include <QWidget>
#include <QLineEdit>
#include <QPushButton>
#include <QComboBox>
#include <QLabel>
#include <QSettings>
#include <QVector>

struct FlowEntry {
    int id;
    QString step;
    QString category;
    int duration;
    qreal focus;
    qreal retention;
    QString technique;
    int order;
    bool active;
    QColor color;
};

class PaperReadingFlow : public QWidget {
    Q_OBJECT
public:
    explicit PaperReadingFlow(QWidget* parent = nullptr);
    void addEntry(const FlowEntry& entry);
    QList<FlowEntry> entries() const;
    int activeCount() const;
    qreal avgFocus() const;
    QMap<QString, int> categoryCounts() const;

signals:
    void flowUpdated(int id, qreal focus);

private slots:
    void onCreate();
    void onClear();

protected:
    void paintEvent(QPaintEvent*) override;

private:
    void setupUI();
    void drawFlowView(QPainter& p, const QRect& rect);
    void drawCategoryChart(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();

    QLineEdit* inputField_;
    QPushButton* createBtn_;
    QPushButton* clearBtn_;
    QComboBox* categoryCombo_;
    QLabel* infoLabel_;
    QSettings settings_;
    QList<FlowEntry> entries_;
};
