#pragma once
#include <QWidget>
#include <QSettings>
#include <QPainter>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>
struct DocumentVaultEntry {
    int id; QString document; QString category; QString storage;
    qreal size; int versions; bool locked; QColor color;
};
class PaperDocumentVault : public QWidget {
    Q_OBJECT
public:
    explicit PaperDocumentVault(QWidget* parent = nullptr);
    void addEntry(const DocumentVaultEntry& entry);
    QList<DocumentVaultEntry> entries() const;
    int lockedCount() const;
    qreal totalSize() const;
    QMap<QString, int> categoryCounts() const;
signals:
    void documentStored(int id, qreal size);
private slots:
    void onStore();
    void onClear();
protected:
    void paintEvent(QPaintEvent*) override;
private:
    void setupUI();
    void drawVaultView(QPainter& p, const QRect& rect);
    void drawCategoryChart(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();
    QList<DocumentVaultEntry> entries_;
    QSettings settings_;
    QPushButton* storeBtn_;
    QPushButton* clearBtn_;
    QComboBox* categoryCombo_;
    QLineEdit* inputField_;
    QLabel* infoLabel_;
};
