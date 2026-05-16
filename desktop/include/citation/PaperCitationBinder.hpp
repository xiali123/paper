#pragma once
#include <QWidget>
#include <QSettings>
#include <QPainter>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>
struct CitationBindEntry {
    int id; QString source; QString category; QString target;
    qreal confidence; int references; bool verified; QColor color;
};
class PaperCitationBinder : public QWidget {
    Q_OBJECT
public:
    explicit PaperCitationBinder(QWidget* parent = nullptr);
    void addEntry(const CitationBindEntry& entry);
    QList<CitationBindEntry> entries() const;
    int verifiedCount() const;
    qreal avgConfidence() const;
    QMap<QString, int> categoryCounts() const;
signals:
    void citationBound(int id, qreal confidence);
private slots:
    void onBind();
    void onClear();
protected:
    void paintEvent(QPaintEvent*) override;
private:
    void setupUI();
    void drawBindView(QPainter& p, const QRect& rect);
    void drawCategoryChart(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();
    QList<CitationBindEntry> entries_;
    QSettings settings_;
    QPushButton* bindBtn_;
    QPushButton* clearBtn_;
    QComboBox* categoryCombo_;
    QLineEdit* inputField_;
    QLabel* infoLabel_;
};
