#pragma once
#include <QWidget>
#include <QSettings>
#include <QPainter>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>
struct CitationBinder2Entry {
    int id; QString paper; QString category; QString style;
    qreal accuracy; int citations; bool verified; QColor color;
};
class PaperCitationBinder2 : public QWidget {
    Q_OBJECT
public:
    explicit PaperCitationBinder2(QWidget* parent = nullptr);
    void addEntry(const CitationBinder2Entry& entry);
    QList<CitationBinder2Entry> entries() const;
    int verifiedCount() const;
    qreal avgAccuracy() const;
    QMap<QString, int> categoryCounts() const;
signals:
    void citationBound(int id, qreal accuracy);
private slots:
    void onBind();
    void onClear();
protected:
    void paintEvent(QPaintEvent*) override;
private:
    void setupUI();
    void drawBinderView(QPainter& p, const QRect& rect);
    void drawCategoryChart(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();
    QList<CitationBinder2Entry> entries_;
    QSettings settings_;
    QPushButton* bindBtn_;
    QPushButton* clearBtn_;
    QComboBox* categoryCombo_;
    QLineEdit* inputField_;
    QLabel* infoLabel_;
};
