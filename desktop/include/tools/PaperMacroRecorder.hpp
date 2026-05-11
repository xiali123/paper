#pragma once
#include <QWidget>
#include <QSettings>
#include <QPainter>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>

struct MacroEntry {
    int id;
    QString name;
    QString category;
    QString trigger;
    int steps;
    int uses;
    QString lastUsed;
    bool active;
    QColor color;
};

class PaperMacroRecorder : public QWidget {
    Q_OBJECT
public:
    explicit PaperMacroRecorder(QWidget* parent = nullptr);
    void addEntry(const MacroEntry& entry);
    QList<MacroEntry> entries() const;
    int activeCount() const;
    int totalUses() const;
    QMap<QString, int> categoryCounts() const;

signals:
    void macroRecorded(int id, const QString& name);

private slots:
    void onRecord();
    void onClear();

protected:
    void paintEvent(QPaintEvent*) override;

private:
    void setupUI();
    void drawMacroList(QPainter& p, const QRect& rect);
    void drawCategoryChart(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();

    QList<MacroEntry> entries_;
    QSettings settings_;
    QPushButton* recordBtn_;
    QPushButton* clearBtn_;
    QComboBox* categoryCombo_;
    QLineEdit* inputField_;
    QLabel* infoLabel_;
};
