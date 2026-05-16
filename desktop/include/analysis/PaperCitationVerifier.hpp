#pragma once
#include <QWidget>
#include <QSettings>
#include <QPainter>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>
struct CitationVerifyEntry {
    int id; QString citation; QString category; QString source;
    qreal confidence; int year; bool valid; bool doiResolved; QColor color;
};
class PaperCitationVerifier : public QWidget {
    Q_OBJECT
public:
    explicit PaperCitationVerifier(QWidget* parent = nullptr);
    void addEntry(const CitationVerifyEntry& entry);
    QList<CitationVerifyEntry> entries() const;
    int validCount() const;
    qreal avgConfidence() const;
    QMap<QString, int> categoryCounts() const;
signals:
    void citationVerified(int id, qreal confidence);
private slots:
    void onVerify();
    void onClear();
protected:
    void paintEvent(QPaintEvent*) override;
private:
    void setupUI();
    void drawCitationList(QPainter& p, const QRect& rect);
    void drawCategoryChart(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();
    QList<CitationVerifyEntry> entries_;
    QSettings settings_;
    QPushButton* verifyBtn_;
    QPushButton* clearBtn_;
    QComboBox* categoryCombo_;
    QLineEdit* inputField_;
    QLabel* infoLabel_;
};
