#pragma once
#include <QWidget>
#include <QSettings>
#include <QPainter>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>
struct CitationMatchEntry {
    int id; QString source; QString category; QString target;
    qreal confidence; int references; bool verified; QColor color;
};
class PaperCitationMatcher : public QWidget {
    Q_OBJECT
public:
    explicit PaperCitationMatcher(QWidget* parent = nullptr);
    void addEntry(const CitationMatchEntry& entry);
    QList<CitationMatchEntry> entries() const;
    int verifiedCount() const;
    qreal avgConfidence() const;
    QMap<QString, int> categoryCounts() const;
signals:
    void matchFound(int id, qreal confidence);
private slots:
    void onMatch();
    void onClear();
protected:
    void paintEvent(QPaintEvent*) override;
private:
    void setupUI();
    void drawMatchList(QPainter& p, const QRect& rect);
    void drawCategoryChart(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();
    QList<CitationMatchEntry> entries_;
    QSettings settings_;
    QPushButton* matchBtn_;
    QPushButton* clearBtn_;
    QComboBox* categoryCombo_;
    QLineEdit* inputField_;
    QLabel* infoLabel_;
};
