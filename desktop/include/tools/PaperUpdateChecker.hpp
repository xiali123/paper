#pragma once
#include <QWidget>
#include <QSettings>
#include <QPainter>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>

struct UpdateEntry {
    int id;
    QString version;
    QString component;
    QString category;
    QString severity;
    QString changelog;
    bool installed;
    bool critical;
    QColor color;
};

class PaperUpdateChecker : public QWidget {
    Q_OBJECT
public:
    explicit PaperUpdateChecker(QWidget* parent = nullptr);
    void addEntry(const UpdateEntry& entry);
    QList<UpdateEntry> entries() const;
    int criticalCount() const;
    int installedCount() const;
    QMap<QString, int> categoryCounts() const;

signals:
    void updateFound(int id, const QString& version);

private slots:
    void onCheck();
    void onClear();

protected:
    void paintEvent(QPaintEvent*) override;

private:
    void setupUI();
    void drawUpdateList(QPainter& p, const QRect& rect);
    void drawCategoryChart(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();

    QList<UpdateEntry> entries_;
    QSettings settings_;
    QPushButton* checkBtn_;
    QPushButton* clearBtn_;
    QComboBox* categoryCombo_;
    QLineEdit* inputField_;
    QLabel* infoLabel_;
};
