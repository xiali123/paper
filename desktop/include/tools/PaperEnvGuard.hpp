#pragma once
#include <QWidget>
#include <QSettings>
#include <QPainter>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>
struct EnvGuardEntry {
    int id; QString variable; QString category; QString environment;
    qreal sensitivity; int references; bool secret; QColor color;
};
class PaperEnvGuard : public QWidget {
    Q_OBJECT
public:
    explicit PaperEnvGuard(QWidget* parent = nullptr);
    void addEntry(const EnvGuardEntry& entry);
    QList<EnvGuardEntry> entries() const;
    int secretCount() const;
    qreal avgSensitivity() const;
    QMap<QString, int> categoryCounts() const;
signals:
    void envChecked(int id, qreal sensitivity);
private slots:
    void onGuard();
    void onClear();
protected:
    void paintEvent(QPaintEvent*) override;
private:
    void setupUI();
    void drawGuardView(QPainter& p, const QRect& rect);
    void drawCategoryChart(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();
    QList<EnvGuardEntry> entries_;
    QSettings settings_;
    QPushButton* guardBtn_;
    QPushButton* clearBtn_;
    QComboBox* categoryCombo_;
    QLineEdit* inputField_;
    QLabel* infoLabel_;
};
