#pragma once
#include <QWidget>
#include <QSettings>
#include <QPainter>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>

struct CitationLogEntry {
    int id;
    QString source;
    QString target;
    QString context;
    QString format;
    QString category;
    bool verified;
    QColor color;
};

class PaperCitationLogger : public QWidget {
    Q_OBJECT
public:
    explicit PaperCitationLogger(QWidget* parent = nullptr);
    void addEntry(const CitationLogEntry& entry);
    QList<CitationLogEntry> entries() const;
    int verifiedCount() const;
    QMap<QString, int> formatCounts() const;
    QMap<QString, int> categoryCounts() const;

signals:
    void citationLogged(int id, const QString& source);

private slots:
    void onLog();
    void onClear();

protected:
    void paintEvent(QPaintEvent*) override;

private:
    void setupUI();
    void drawCitationList(QPainter& p, const QRect& rect);
    void drawFormatChart(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();

    QList<CitationLogEntry> entries_;
    QSettings settings_;
    QPushButton* logBtn_;
    QPushButton* clearBtn_;
    QComboBox* formatCombo_;
    QLineEdit* inputField_;
    QLabel* infoLabel_;
};
