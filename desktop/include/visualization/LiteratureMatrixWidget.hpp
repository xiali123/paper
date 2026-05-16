#pragma once

#include <QWidget>
#include <QPainter>
#include <QLabel>
#include <QPushButton>
#include <QComboBox>
#include <QTableWidget>
#include <QList>
#include <QMap>
#include <QSettings>

struct MatrixCell {
    int row{-1};
    int col{-1};
    QString value;
    QColor color;
    QString tooltip;
};

class LiteratureMatrixWidget : public QWidget {
    Q_OBJECT

public:
    explicit LiteratureMatrixWidget(QWidget* parent = nullptr);

    void setPapers(const QList<QPair<int, QString>>& papers);
    void setCriteria(const QStringList& criteria);
    void setCell(int row, int col, const QString& value, const QColor& color = Qt::white);
    void addDefaultCriteria();
    void autoFill();
    MatrixCell cell(int row, int col) const;

signals:
    void cellClicked(int row, int col, const QString& paper, const QString& criterion);
    void matrixUpdated(int rows, int cols);

private slots:
    void onModeChanged(int index);
    void onCellClicked(int row, int col);
    void onExport();
    void onReset();

private:
    void setupUI();
    void paintEvent(QPaintEvent* event) override;
    void refreshTable();
    void updateStats();
    void loadSettings();
    void saveSettings();

    QTableWidget* matrixTable_{nullptr};
    QComboBox* modeCombo_{nullptr};
    QPushButton* exportBtn_{nullptr};
    QPushButton* resetBtn_{nullptr};
    QLabel* statsLabel_{nullptr};

    QStringList papers_;
    QStringList criteria_;
    QMap<QPair<int,int>, MatrixCell> cells_;
    QSettings settings_;
};
