#pragma once
#include <QWidget>
#include <QSettings>
#include <QPainter>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>

struct AnnotationEntry {
    int id;
    QString text;
    QString highlight;
    QString tag;
    QString category;
    QString page;
    qreal confidence;
    bool starred;
    QColor color;
};

class PaperAnnotationStudio : public QWidget {
    Q_OBJECT
public:
    explicit PaperAnnotationStudio(QWidget* parent = nullptr);
    void addEntry(const AnnotationEntry& entry);
    QList<AnnotationEntry> entries() const;
    int starredCount() const;
    qreal avgConfidence() const;
    QMap<QString, int> categoryCounts() const;

signals:
    void annotationCreated(int id, qreal confidence);

private slots:
    void onCreate();
    void onClear();

protected:
    void paintEvent(QPaintEvent*) override;

private:
    void setupUI();
    void drawAnnotationList(QPainter& p, const QRect& rect);
    void drawCategoryChart(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();

    QList<AnnotationEntry> entries_;
    QSettings settings_;
    QPushButton* createBtn_;
    QPushButton* clearBtn_;
    QComboBox* categoryCombo_;
    QLineEdit* inputField_;
    QLabel* infoLabel_;
};
