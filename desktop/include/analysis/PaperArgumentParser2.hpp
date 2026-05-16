#pragma once
#include <QWidget>
#include <QSettings>
#include <QPainter>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>
struct ArgumentParseEntry {
    int id; QString premise; QString category; QString conclusion;
    qreal validity; int steps; bool sound; QColor color;
};
class PaperArgumentParser2 : public QWidget {
    Q_OBJECT
public:
    explicit PaperArgumentParser2(QWidget* parent = nullptr);
    void addEntry(const ArgumentParseEntry& entry);
    QList<ArgumentParseEntry> entries() const;
    int soundCount() const;
    qreal avgValidity() const;
    QMap<QString, int> categoryCounts() const;
signals:
    void argumentParsed(int id, qreal validity);
private slots:
    void onParse();
    void onClear();
protected:
    void paintEvent(QPaintEvent*) override;
private:
    void setupUI();
    void drawArgumentTree(QPainter& p, const QRect& rect);
    void drawCategoryChart(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();
    QList<ArgumentParseEntry> entries_;
    QSettings settings_;
    QPushButton* parseBtn_;
    QPushButton* clearBtn_;
    QComboBox* categoryCombo_;
    QLineEdit* inputField_;
    QLabel* infoLabel_;
};
