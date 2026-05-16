#pragma once
#include <QWidget>
#include <QLineEdit>
#include <QPushButton>
#include <QComboBox>
#include <QLabel>
#include <QSettings>
#include <QVector>

struct ScriptEntry {
    int id;
    QString scriptName;
    QString language;
    QString status;
    int runtime;
    qreal memory;
    QString output;
    bool success;
    QColor color;
};

class PaperScriptRunner : public QWidget {
    Q_OBJECT
public:
    explicit PaperScriptRunner(QWidget* parent = nullptr);
    void addEntry(const ScriptEntry& entry);
    QList<ScriptEntry> entries() const;
    int successCount() const;
    qreal avgRuntime() const;
    QMap<QString, int> languageCounts() const;

signals:
    void scriptExecuted(int id, int runtime);

private slots:
    void onRun();
    void onClear();

protected:
    void paintEvent(QPaintEvent*) override;

private:
    void setupUI();
    void drawScriptList(QPainter& p, const QRect& rect);
    void drawLanguageChart(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();

    QLineEdit* inputField_;
    QPushButton* runBtn_;
    QPushButton* clearBtn_;
    QComboBox* langCombo_;
    QLabel* infoLabel_;
    QSettings settings_;
    QList<ScriptEntry> entries_;
};
