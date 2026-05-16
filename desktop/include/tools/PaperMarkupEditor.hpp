#pragma once
#include <QWidget>
#include <QSettings>
#include <QPainter>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>

struct MarkupEntry {
    int id;
    QString title;
    QString format;
    QString category;
    QString element;
    int lines;
    int chars;
    bool rendered;
    QColor color;
};

class PaperMarkupEditor : public QWidget {
    Q_OBJECT
public:
    explicit PaperMarkupEditor(QWidget* parent = nullptr);
    void addEntry(const MarkupEntry& entry);
    QList<MarkupEntry> entries() const;
    int renderedCount() const;
    qreal avgChars() const;
    QMap<QString, int> categoryCounts() const;

signals:
    void markupCreated(int id, const QString& format);

private slots:
    void onCreate();
    void onClear();

protected:
    void paintEvent(QPaintEvent*) override;

private:
    void setupUI();
    void drawMarkupList(QPainter& p, const QRect& rect);
    void drawCategoryChart(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();

    QList<MarkupEntry> entries_;
    QSettings settings_;
    QPushButton* createBtn_;
    QPushButton* clearBtn_;
    QComboBox* categoryCombo_;
    QLineEdit* inputField_;
    QLabel* infoLabel_;
};
