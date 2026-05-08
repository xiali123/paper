#pragma once

#include <QWidget>
#include <QLabel>
#include <QPushButton>
#include <QComboBox>
#include <QListWidget>
#include <QTextEdit>
#include <QProgressBar>
#include <QList>
#include <QMap>
#include <QSettings>

struct ExportJob {
    int id{-1};
    QString name;
    QString format;
    QStringList paperIds;
    QString outputPath;
    QString status{"pending"};
    int progress{0};
    int totalCount{0};
    qint64 createdAt{0};
};

class PaperExportBatch : public QWidget {
    Q_OBJECT

public:
    explicit PaperExportBatch(QWidget* parent = nullptr);

    void addJob(const ExportJob& job);
    void removeJob(int jobId);
    QList<ExportJob> jobs() const;
    void setPapers(const QList<QPair<int, QString>>& papers);

signals:
    void jobStarted(int jobId);
    void jobCompleted(int jobId, const QString& outputPath);
    void jobFailed(int jobId, const QString& error);

private slots:
    void onCreateJob();
    void onDeleteJob();
    void onRunAll();
    void onRunSelected();
    void onJobSelected();
    void onFormatChanged(int index);
    void onSelectAll();
    void onSelectNone();

private:
    void setupUI();
    void refreshJobList();
    void refreshPaperList();
    void updateStats();
    void loadSettings();
    void saveSettings();
    void simulateExport(ExportJob& job);

    QListWidget* paperList_{nullptr};
    QListWidget* jobList_{nullptr};
    QComboBox* formatCombo_{nullptr};
    QLineEdit* nameEdit_{nullptr};
    QLineEdit* pathEdit_{nullptr};
    QProgressBar* progressBar_{nullptr};
    QPushButton* createBtn_{nullptr};
    QPushButton* deleteBtn_{nullptr};
    QPushButton* runAllBtn_{nullptr};
    QPushButton* runSelectedBtn_{nullptr};
    QPushButton* selectAllBtn_{nullptr};
    QPushButton* selectNoneBtn_{nullptr};
    QLabel* statsLabel_{nullptr};

    QList<ExportJob> jobs_;
    QList<QPair<int, QString>> papers_;
    int nextId_{1};
    int selectedJobId_{-1};
};
