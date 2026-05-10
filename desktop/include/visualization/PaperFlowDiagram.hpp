#pragma once
#include <QWidget>
#include <QSettings>
#include <QPainter>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>

struct FlowEntry {
    int id;
    QString source;
    QString target;
    QString category;
    qreal value;
    qreal capacity;
    bool active;
    QColor color;
};

class PaperFlowDiagram : public QWidget {
    Q_OBJECT
public:
    explicit PaperFlowDiagram(QWidget* parent = nullptr);
    void addEntry(const FlowEntry& entry);
    QList<FlowEntry> entries() const;
    qreal totalFlow() const;
    int activeCount() const;
    QMap<QString, int> categoryCounts() const;

signals:
    void flowUpdated(int id, qreal value);

private slots:
    void onGenerate();
    void onClear();

protected:
    void paintEvent(QPaintEvent*) override;

private:
    void setupUI();
    void drawFlowView(QPainter& p, const QRect& rect);
    void drawCategoryLegend(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();

    QList<FlowEntry> entries_;
    QSettings settings_;
    QPushButton* generateBtn_;
    QPushButton* clearBtn_;
    QComboBox* categoryCombo_;
    QLineEdit* inputField_;
    QLabel* infoLabel_;
};
