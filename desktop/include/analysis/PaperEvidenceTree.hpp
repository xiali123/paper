#pragma once
#include <QWidget>
#include <QSettings>
#include <QPainter>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>
struct EvidenceNode {
    int id; QString claim; QString category; QString source;
    qreal confidence; int supporting; bool verified; QColor color;
};
class PaperEvidenceTree : public QWidget {
    Q_OBJECT
public:
    explicit PaperEvidenceTree(QWidget* parent = nullptr);
    void addEntry(const EvidenceNode& entry);
    QList<EvidenceNode> entries() const;
    int verifiedCount() const;
    qreal avgConfidence() const;
    QMap<QString, int> categoryCounts() const;
signals:
    void evidenceLinked(int id, qreal confidence);
private slots:
    void onBuild();
    void onClear();
protected:
    void paintEvent(QPaintEvent*) override;
private:
    void setupUI();
    void drawTree(QPainter& p, const QRect& rect);
    void drawCategoryChart(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();
    QList<EvidenceNode> entries_;
    QSettings settings_;
    QPushButton* buildBtn_;
    QPushButton* clearBtn_;
    QComboBox* categoryCombo_;
    QLineEdit* inputField_;
    QLabel* infoLabel_;
};
