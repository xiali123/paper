#pragma once
#include <QWidget>
#include <QSettings>
#include <QPainter>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>
struct ClipEntry {
    int id; QString text; QString category; QString source;
    int uses; qreal relevance; QString date; bool pinned; QColor color;
};
class PaperClipboardManager : public QWidget {
    Q_OBJECT
public:
    explicit PaperClipboardManager(QWidget* parent = nullptr);
    void addEntry(const ClipEntry& entry);
    QList<ClipEntry> entries() const;
    int pinnedCount() const;
    qreal avgRelevance() const;
    QMap<QString, int> categoryCounts() const;
signals:
    void clipSaved(int id, qreal relevance);
private slots:
    void onSave();
    void onClear();
protected:
    void paintEvent(QPaintEvent*) override;
private:
    void setupUI();
    void drawClipList(QPainter& p, const QRect& rect);
    void drawCategoryChart(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();
    QList<ClipEntry> entries_;
    QSettings settings_;
    QPushButton* saveBtn_;
    QPushButton* clearBtn_;
    QComboBox* categoryCombo_;
    QLineEdit* inputField_;
    QLabel* infoLabel_;
};
