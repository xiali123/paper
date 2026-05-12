#pragma once
#include <QWidget>
#include <QSettings>
#include <QPainter>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>
struct EnvVarEntry {
    int id; QString key; QString category; QString value;
    qreal length; int usage; bool secret; QColor color;
};
class PaperEnvVarManager : public QWidget {
    Q_OBJECT
public:
    explicit PaperEnvVarManager(QWidget* parent = nullptr);
    void addEntry(const EnvVarEntry& entry);
    QList<EnvVarEntry> entries() const;
    int secretCount() const;
    qreal avgLength() const;
    QMap<QString, int> categoryCounts() const;
signals:
    void varAdded(int id, qreal length);
private slots:
    void onAdd();
    void onClear();
protected:
    void paintEvent(QPaintEvent*) override;
private:
    void setupUI();
    void drawVarList(QPainter& p, const QRect& rect);
    void drawCategoryChart(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();
    QList<EnvVarEntry> entries_;
    QSettings settings_;
    QPushButton* addBtn_;
    QPushButton* clearBtn_;
    QComboBox* categoryCombo_;
    QLineEdit* inputField_;
    QLabel* infoLabel_;
};
