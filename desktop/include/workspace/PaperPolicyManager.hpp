#pragma once
#include <QWidget>
#include <QSettings>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>
#include <QVector>

struct PolicyEntry {
    int id;
    QString policyName;
    QString type;
    QString scope;
    qreal enforcement;
    int violations;
    QString status;
    QString lastUpdated;
    QString category;
    bool active;
    QColor color;
};

class PaperPolicyManager : public QWidget {
    Q_OBJECT
public:
    explicit PaperPolicyManager(QWidget* parent = nullptr);
    void addEntry(const PolicyEntry& entry);
    QList<PolicyEntry> entries() const;
    qreal avgEnforcement() const;
    int activeCount() const;
    QMap<QString, int> typeCounts() const;

signals:
    void policyManaged(int id, qreal enforcement);

private slots:
    void onManage();
    void onClear();

protected:
    void paintEvent(QPaintEvent*) override;

private:
    void setupUI();
    void drawPolicyList(QPainter& p, const QRect& rect);
    void drawTypeChart(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();

    QSettings settings_;
    QVector<PolicyEntry> entries_;
    QPushButton* manageBtn_;
    QPushButton* clearBtn_;
    QComboBox* filterCombo_;
    QLineEdit* inputField_;
    QLabel* infoLabel_;
};
