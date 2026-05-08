#pragma once

#include <QWidget>
#include <QLabel>
#include <QPushButton>
#include <QComboBox>
#include <QListWidget>
#include <QList>
#include <QMap>
#include <QSettings>

struct FigureEntry {
    int id{-1};
    int paperId{-1};
    QString caption;
    QString figureType{"figure"};
    int number{0};
    QString description;
    QString filePath;
    QString pageContext;
    QStringList tags;
};

class PaperFigureExtractor : public QWidget {
    Q_OBJECT

public:
    explicit PaperFigureExtractor(QWidget* parent = nullptr);

    void setPaper(int paperId, const QString& title);
    void addFigure(const FigureEntry& fig);
    void removeFigure(int figId);
    QList<FigureEntry> figures() const;
    QList<FigureEntry> figuresByType(const QString& type) const;
    void exportCatalog(const QString& path);

signals:
    void figureClicked(int figId, const QString& caption);
    void figureExported(int count);

private slots:
    void onAdd();
    void onDelete();
    void onExport();
    void onFilterChanged(int index);
    void onFigureSelected();

private:
    void setupUI();
    void refreshList();
    void updateStats();
    void loadSettings();
    void saveSettings();

    QListWidget* figureList_{nullptr};
    QTextEdit* descEdit_{nullptr};
    QComboBox* typeCombo_{nullptr};
    QComboBox* filterCombo_{nullptr};
    QLineEdit* captionEdit_{nullptr};
    QLineEdit* numberEdit_{nullptr};
    QPushButton* addBtn_{nullptr};
    QPushButton* deleteBtn_{nullptr};
    QPushButton* exportBtn_{nullptr};
    QLabel* statsLabel_{nullptr};
    QLabel* paperLabel_{nullptr};

    QList<FigureEntry> figures_;
    int nextId_{1};
    int currentPaperId_{-1};
    int selectedId_{-1};
};
