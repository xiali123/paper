#pragma once
#include <QWidget>
#include <QSettings>
#include <QPushButton>
#include <QLabel>
#include <QLineEdit>
#include <QComboBox>

struct AuthorEntry {
    int id;
    QString name;
    QString variant;
    QString affiliation;
    int paperCount;
    qreal similarity;
    QString cluster;
    QString orcidHint;
    QColor color;
};

class PaperAuthorDisambiguator : public QWidget {
    Q_OBJECT
public:
    explicit PaperAuthorDisambiguator(QWidget* parent = nullptr);
    void addAuthor(const AuthorEntry& entry);
    QList<AuthorEntry> authors() const;
    int uniqueAuthors() const;
    int clusterCount() const;
    qreal avgSimilarity() const;

signals:
    void authorDisambiguated(int id, const QString& cluster);

protected:
    void paintEvent(QPaintEvent*) override;

private:
    void setupUI();
    void onAnalyze();
    void onClear();
    void drawAuthorList(QPainter& p, const QRect& rect);
    void drawClusterChart(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();

    QSettings settings_;
    QList<AuthorEntry> authors_;
    QPushButton* analyzeBtn_;
    QPushButton* clearBtn_;
    QLineEdit* inputField_;
    QComboBox* filterCombo_;
    QLabel* infoLabel_;
};
