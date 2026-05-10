#pragma once
#include <QWidget>
#include <QLineEdit>
#include <QPushButton>
#include <QComboBox>
#include <QLabel>
#include <QSettings>
#include <QVector>

struct ThemeEntry {
    int id;
    QString themeName;
    QString baseColor;
    QString accentColor;
    QString mode;
    int fonts;
    qreal contrast;
    bool dark;
    bool custom;
    QColor color;
};

class PaperThemeBuilder : public QWidget {
    Q_OBJECT
public:
    explicit PaperThemeBuilder(QWidget* parent = nullptr);
    void addEntry(const ThemeEntry& entry);
    QList<ThemeEntry> entries() const;
    int darkCount() const;
    int customCount() const;
    QMap<QString, int> modeCounts() const;

signals:
    void themeBuilt(int id, QString themeName);

private slots:
    void onBuild();
    void onClear();

protected:
    void paintEvent(QPaintEvent*) override;

private:
    void setupUI();
    void drawThemeList(QPainter& p, const QRect& rect);
    void drawModeChart(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();

    QLineEdit* inputField_;
    QPushButton* buildBtn_;
    QPushButton* clearBtn_;
    QComboBox* modeCombo_;
    QLabel* infoLabel_;
    QSettings settings_;
    QList<ThemeEntry> entries_;
};
