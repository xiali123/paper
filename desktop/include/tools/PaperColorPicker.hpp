#pragma once
#include <QWidget>
#include <QSettings>
#include <QPainter>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>

struct ColorEntry {
    int id;
    QString name;
    QString category;
    QString hex;
    int r, g, b;
    int uses;
    bool favorite;
    QColor color;
};

class PaperColorPicker : public QWidget {
    Q_OBJECT
public:
    explicit PaperColorPicker(QWidget* parent = nullptr);
    void addEntry(const ColorEntry& entry);
    QList<ColorEntry> entries() const;
    int favoriteCount() const;
    int totalUses() const;
    QMap<QString, int> categoryCounts() const;

signals:
    void colorPicked(int id, const QString& hex);

private slots:
    void onPick();
    void onClear();

protected:
    void paintEvent(QPaintEvent*) override;

private:
    void setupUI();
    void drawColorGrid(QPainter& p, const QRect& rect);
    void drawCategoryChart(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();

    QList<ColorEntry> entries_;
    QSettings settings_;
    QPushButton* pickBtn_;
    QPushButton* clearBtn_;
    QComboBox* categoryCombo_;
    QLineEdit* inputField_;
    QLabel* infoLabel_;
};
