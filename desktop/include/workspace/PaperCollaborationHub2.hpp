#pragma once
#include <QWidget>
#include <QSettings>
#include <QPainter>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>
struct CollaborationHub2Entry {
    int id; QString project; QString category; QString member;
    qreal contribution; int commits; bool active; QColor color;
};
class PaperCollaborationHub2 : public QWidget {
    Q_OBJECT
public:
    explicit PaperCollaborationHub2(QWidget* parent = nullptr);
    void addEntry(const CollaborationHub2Entry& entry);
    QList<CollaborationHub2Entry> entries() const;
    int activeCount() const;
    qreal avgContribution() const;
    QMap<QString, int> categoryCounts() const;
signals:
    void collaborationUpdated(int id, qreal contribution);
private slots:
    void onRefresh();
    void onClear();
protected:
    void paintEvent(QPaintEvent*) override;
private:
    void setupUI();
    void drawHubView(QPainter& p, const QRect& rect);
    void drawCategoryChart(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();
    QList<CollaborationHub2Entry> entries_;
    QSettings settings_;
    QPushButton* refreshBtn_;
    QPushButton* clearBtn_;
    QComboBox* categoryCombo_;
    QLineEdit* inputField_;
    QLabel* infoLabel_;
};
