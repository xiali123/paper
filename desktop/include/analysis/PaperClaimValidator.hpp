#pragma once
#include <QWidget>
#include <QSettings>
#include <QPainter>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>

struct ClaimEntry {
    int id;
    QString claim;
    QString evidence;
    QString category;
    QString strength;
    qreal score;
    int sources;
    bool verified;
    QColor color;
};

class PaperClaimValidator : public QWidget {
    Q_OBJECT
public:
    explicit PaperClaimValidator(QWidget* parent = nullptr);
    void addEntry(const ClaimEntry& entry);
    QList<ClaimEntry> entries() const;
    int verifiedCount() const;
    qreal avgScore() const;
    QMap<QString, int> categoryCounts() const;

signals:
    void claimValidated(int id, qreal score);

private slots:
    void onValidate();
    void onClear();

protected:
    void paintEvent(QPaintEvent*) override;

private:
    void setupUI();
    void drawClaimList(QPainter& p, const QRect& rect);
    void drawCategoryChart(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();

    QList<ClaimEntry> entries_;
    QSettings settings_;
    QPushButton* validateBtn_;
    QPushButton* clearBtn_;
    QComboBox* categoryCombo_;
    QLineEdit* inputField_;
    QLabel* infoLabel_;
};
