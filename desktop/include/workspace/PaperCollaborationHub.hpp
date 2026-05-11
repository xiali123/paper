#pragma once
#include <QWidget>
#include <QSettings>
#include <QPainter>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>
struct CollabEntry {
    int id; QString member; QString category; QString role;
    int contributions; qreal activity; QString joined; bool active; QColor color;
};
class PaperCollaborationHub : public QWidget {
    Q_OBJECT
public:
    explicit PaperCollaborationHub(QWidget* parent = nullptr);
    void addEntry(const CollabEntry& entry);
    QList<CollabEntry> entries() const;
    int activeCount() const;
    qreal avgActivity() const;
    QMap<QString, int> categoryCounts() const;
signals:
    void memberAdded(int id, qreal activity);
private slots:
    void onAdd();
    void onClear();
protected:
    void paintEvent(QPaintEvent*) override;
private:
    void setupUI();
    void drawMemberList(QPainter& p, const QRect& rect);
    void drawCategoryChart(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();
    QList<CollabEntry> entries_;
    QSettings settings_;
    QPushButton* addBtn_;
    QPushButton* clearBtn_;
    QComboBox* categoryCombo_;
    QLineEdit* inputField_;
    QLabel* infoLabel_;
};
