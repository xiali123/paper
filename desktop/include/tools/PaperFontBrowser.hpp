#pragma once
#include <QWidget>
#include <QSettings>
#include <QPainter>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>

struct FontEntry {
    int id;
    QString name;
    QString category;
    QString style;
    int weight;
    int size;
    int usage;
    bool favorite;
    QColor color;
};

class PaperFontBrowser : public QWidget {
    Q_OBJECT
public:
    explicit PaperFontBrowser(QWidget* parent = nullptr);
    void addEntry(const FontEntry& entry);
    QList<FontEntry> entries() const;
    int favoriteCount() const;
    int totalUsage() const;
    QMap<QString, int> categoryCounts() const;

signals:
    void fontSelected(int id, const QString& name);

private slots:
    void onBrowse();
    void onClear();

protected:
    void paintEvent(QPaintEvent*) override;

private:
    void setupUI();
    void drawFontList(QPainter& p, const QRect& rect);
    void drawCategoryChart(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();

    QList<FontEntry> entries_;
    QSettings settings_;
    QPushButton* browseBtn_;
    QPushButton* clearBtn_;
    QComboBox* categoryCombo_;
    QLineEdit* inputField_;
    QLabel* infoLabel_;
};
