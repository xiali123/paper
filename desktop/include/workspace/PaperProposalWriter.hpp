#pragma once
#include <QWidget>
#include <QSettings>
#include <QPainter>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>
struct ProposalWriterEntry {
    int id; QString title; QString category; QString status;
    qreal completeness; int sections; bool submitted; QColor color;
};
class PaperProposalWriter : public QWidget {
    Q_OBJECT
public:
    explicit PaperProposalWriter(QWidget* parent = nullptr);
    void addEntry(const ProposalWriterEntry& entry);
    QList<ProposalWriterEntry> entries() const;
    int submittedCount() const;
    qreal avgCompleteness() const;
    QMap<QString, int> categoryCounts() const;
signals:
    void proposalSubmitted(int id, qreal completeness);
private slots:
    void onAdd();
    void onClear();
protected:
    void paintEvent(QPaintEvent*) override;
private:
    void setupUI();
    void drawProposalView(QPainter& p, const QRect& rect);
    void drawCategoryChart(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();
    QList<ProposalWriterEntry> entries_;
    QSettings settings_;
    QPushButton* addBtn_;
    QPushButton* clearBtn_;
    QComboBox* categoryCombo_;
    QLineEdit* inputField_;
    QLabel* infoLabel_;
};
