#pragma once
#include <QWidget>
#include <QLineEdit>
#include <QPushButton>
#include <QComboBox>
#include <QLabel>
#include <QSettings>
#include <QVector>

struct ContextMenuEntry {
    int id;
    QString action;
    QString menu;
    QString shortcut;
    QString icon;
    int position;
    bool enabled;
    bool separator;
    QColor color;
};

class PaperContextMenuEditor : public QWidget {
    Q_OBJECT
public:
    explicit PaperContextMenuEditor(QWidget* parent = nullptr);
    void addEntry(const ContextMenuEntry& entry);
    QList<ContextMenuEntry> entries() const;
    int enabledCount() const;
    int separatorCount() const;
    QMap<QString, int> menuCounts() const;

signals:
    void menuEdited(int id, QString action);

private slots:
    void onAdd();
    void onClear();

protected:
    void paintEvent(QPaintEvent*) override;

private:
    void setupUI();
    void drawMenuList(QPainter& p, const QRect& rect);
    void drawMenuChart(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();

    QLineEdit* inputField_;
    QPushButton* addBtn_;
    QPushButton* clearBtn_;
    QComboBox* menuCombo_;
    QLabel* infoLabel_;
    QSettings settings_;
    QList<ContextMenuEntry> entries_;
};
