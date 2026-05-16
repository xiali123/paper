#pragma once
#include <QWidget>
#include <QLineEdit>
#include <QComboBox>
#include <QPushButton>
#include <QLabel>
#include <QSettings>
#include <QPainter>
#include <QList>

struct FigureCaptionEntry {
    int id;
    QString paperTitle;
    int figureNum;
    QString caption;
    QString figureType;
    qreal confidence;
    QString suggestedCaption;
    int wordCount;
    bool hasCaption;
    QColor color;
};

class PaperFigureCaptioner : public QWidget {
    Q_OBJECT
public:
    explicit PaperFigureCaptioner(QWidget* parent = nullptr);
    void addEntry(const FigureCaptionEntry& entry);
    QList<FigureCaptionEntry> entries() const;
    int missingCaptions() const;
    qreal avgConfidence() const;
    QMap<QString, int> typeCounts() const;

signals:
    void captionGenerated(int id, const QString& caption);

protected:
    void paintEvent(QPaintEvent*) override;

private:
    void setupUI();
    void onGenerate();
    void onClear();
    void drawCaptionList(QPainter& p, const QRect& rect);
    void drawTypeChart(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();

    QLineEdit* inputField_;
    QComboBox* typeCombo_;
    QPushButton* generateBtn_;
    QPushButton* clearBtn_;
    QLabel* infoLabel_;
    QList<FigureCaptionEntry> entries_;
    QSettings settings_;
};
