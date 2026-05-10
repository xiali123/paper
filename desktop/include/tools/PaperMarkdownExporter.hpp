#pragma once
#include <QWidget>
#include <QLineEdit>
#include <QComboBox>
#include <QPushButton>
#include <QLabel>
#include <QSettings>
#include <QPainter>
#include <QList>

struct MarkdownExportEntry {
    int id;
    QString paperTitle;
    QString section;
    QString format;
    int wordCount;
    qreal quality;
    QString template_;
    bool includeImages;
    QString outputPath;
    QColor color;
};

class PaperMarkdownExporter : public QWidget {
    Q_OBJECT
public:
    explicit PaperMarkdownExporter(QWidget* parent = nullptr);
    void addEntry(const MarkdownExportEntry& entry);
    QList<MarkdownExportEntry> entries() const;
    int totalWords() const;
    qreal avgQuality() const;
    QMap<QString, int> formatCounts() const;

signals:
    void exportCompleted(int id, const QString& path);

protected:
    void paintEvent(QPaintEvent*) override;

private:
    void setupUI();
    void onExport();
    void onClear();
    void drawExportList(QPainter& p, const QRect& rect);
    void drawFormatChart(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();

    QLineEdit* inputField_;
    QComboBox* formatCombo_;
    QPushButton* exportBtn_;
    QPushButton* clearBtn_;
    QLabel* infoLabel_;
    QList<MarkdownExportEntry> entries_;
    QSettings settings_;
};
