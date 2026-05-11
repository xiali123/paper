#pragma once
#include <QWidget>
#include <QSettings>
#include <QPainter>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>

struct LabEntry {
    int id;
    QString experiment;
    QString category;
    QString result;
    QString status;
    qreal confidence;
    QString date;
    bool success;
    QColor color;
};

class PaperLabNotebook : public QWidget {
    Q_OBJECT
public:
    explicit PaperLabNotebook(QWidget* parent = nullptr);
    void addEntry(const LabEntry& entry);
    QList<LabEntry> entries() const;
    int successCount() const;
    qreal avgConfidence() const;
    QMap<QString, int> categoryCounts() const;

signals:
    void experimentLogged(int id, qreal confidence);

private slots:
    void onLog();
    void onClear();

protected:
    void paintEvent(QPaintEvent*) override;

private:
    void setupUI();
    void drawLabList(QPainter& p, const QRect& rect);
    void drawCategoryChart(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();

    QList<LabEntry> entries_;
    QSettings settings_;
    QPushButton* logBtn_;
    QPushButton* clearBtn_;
    QComboBox* categoryCombo_;
    QLineEdit* inputField_;
    QLabel* infoLabel_;
};
