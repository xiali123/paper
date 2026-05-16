#pragma once
#include <QWidget>
#include <QSettings>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>
#include <QVector>

struct FeatureFlagEntry {
    int id;
    QString flagName;
    QString environment;
    bool enabled;
    QString description;
    QString rollout;
    int userPercentage;
    QString createdDate;
    QString category;
    bool stable;
    QColor color;
};

class PaperFeatureFlagManager : public QWidget {
    Q_OBJECT
public:
    explicit PaperFeatureFlagManager(QWidget* parent = nullptr);
    void addEntry(const FeatureFlagEntry& entry);
    QList<FeatureFlagEntry> entries() const;
    int enabledCount() const;
    int stableCount() const;
    QMap<QString, int> categoryCounts() const;

signals:
    void flagToggled(int id, bool enabled);

private slots:
    void onToggle();
    void onClear();

protected:
    void paintEvent(QPaintEvent*) override;

private:
    void setupUI();
    void drawFlagList(QPainter& p, const QRect& rect);
    void drawCategoryChart(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();

    QSettings settings_;
    QVector<FeatureFlagEntry> entries_;
    QPushButton* toggleBtn_;
    QPushButton* clearBtn_;
    QComboBox* envCombo_;
    QLineEdit* inputField_;
    QLabel* infoLabel_;
};
