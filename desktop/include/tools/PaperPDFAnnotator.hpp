#pragma once
#include <QWidget>
#include <QLineEdit>
#include <QComboBox>
#include <QPushButton>
#include <QLabel>
#include <QSettings>
#include <QPainter>
#include <QList>

struct PDFAnnotEntry {
    int id;
    QString paperTitle;
    int pageNum;
    QString annotationType;
    QString content;
    QString colorTag;
    qreal x;
    qreal y;
    QString category;
    bool exported;
    QColor color;
};

class PaperPDFAnnotator : public QWidget {
    Q_OBJECT
public:
    explicit PaperPDFAnnotator(QWidget* parent = nullptr);
    void addEntry(const PDFAnnotEntry& entry);
    QList<PDFAnnotEntry> entries() const;
    int exportedCount() const;
    QMap<QString, int> typeCounts() const;
    QMap<QString, int> categoryCounts() const;

signals:
    void annotationAdded(int id, const QString& type);

protected:
    void paintEvent(QPaintEvent*) override;

private:
    void setupUI();
    void onAnnotate();
    void onClear();
    void drawAnnotationList(QPainter& p, const QRect& rect);
    void drawTypeChart(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();

    QLineEdit* inputField_;
    QComboBox* typeCombo_;
    QPushButton* annotateBtn_;
    QPushButton* clearBtn_;
    QLabel* infoLabel_;
    QList<PDFAnnotEntry> entries_;
    QSettings settings_;
};
