#pragma once
#include <QWidget>
#include <QList>
#include <QMap>
#include <QSettings>

struct ProposalEntry {
    int id;
    QString title;
    QString status;
    QString funder;
    qreal amount;
    QString deadline;
    qreal score;
    QString category;
    int papersPlanned;
    bool submitted;
    QColor color;
};

class PaperProposalTracker : public QWidget {
    Q_OBJECT
public:
    explicit PaperProposalTracker(QWidget* parent = nullptr);
    void addEntry(const ProposalEntry& entry);
    QList<ProposalEntry> entries() const;
    qreal totalAmount() const;
    int submittedCount() const;
    QMap<QString, int> categoryCounts() const;
signals:
    void proposalTracked(int id, qreal amount);
private slots:
    void onAdd();
    void onClear();
private:
    void setupUI();
    void paintEvent(QPaintEvent*) override;
    void drawProposalList(QPainter& p, const QRect& rect);
    void drawCategoryChart(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();
    QList<ProposalEntry> entries_;
    QSettings settings_;
    QPushButton* addBtn_;
    QPushButton* clearBtn_;
    QComboBox* statusCombo_;
    QLineEdit* inputField_;
    QLabel* infoLabel_;
};
