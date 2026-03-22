#pragma once

#include <QObject>
#include <QList>
#include <QString>
#include <QFuture>
#include <QThreadPool>

#include "database/LocalDatabase.hpp"

/**
 * @brief 爬虫任务状态
 */
enum class CrawlerStatus {
    Idle,
    Running,
    Completed,
    Failed
};

/**
 * @brief 论文来源
 */
enum class PaperSource {
    ArXiv,
    SemanticScholar,
    PubMed,
    IEEE,
    GoogleScholar,
    Manual
};

/**
 * @brief 爬虫配置
 */
struct CrawlerConfig {
    int maxResults{100};           // 每个源最多抓取数量
    int timeoutMs{30000};          // 请求超时
    int maxConcurrent{5};          // 最大并发数
    bool enableDedup{true};        // 启用去重
    QList<PaperSource> sources{    // 数据源优先级
        PaperSource::ArXiv,
        PaperSource::SemanticScholar,
        PaperSource::PubMed
    };
};

/**
 * @brief 爬虫结果
 */
struct CrawlerResult {
    QList<Paper> papers;
    int totalFound{0};
    int duplicates{0};
    qint64 elapsedMs{0};
    QString error;
};

/**
 * @brief 爬虫引擎基类
 *
 * 功能：
 * - 多源并发爬取
 * - 自动去重
 * - 限流控制
 * - 错误重试
 */
class CrawlerEngine : public QObject {
    Q_OBJECT

public:
    explicit CrawlerEngine(QObject* parent = nullptr);
    ~CrawlerEngine();

    // 配置
    void setConfig(const CrawlerConfig& config);
    CrawlerConfig config() const { return config_; }

    void setDatabase(LocalDatabase* db) { localDb_ = db; }

    // 爬取
    QFuture<CrawlerResult> search(const QString& keyword);
    void searchAsync(const QString& keyword);

    // 状态
    CrawlerStatus status() const { return status_; }
    bool isRunning() const { return status_ == CrawlerStatus::Running; }

signals:
    void progressChanged(int current, int total);
    void paperFound(const Paper& paper);
    void statusChanged(CrawlerStatus status);
    void searchCompleted(const CrawlerResult& result);
    void searchFailed(const QString& error);

private slots:
    void onSourceCompleted(const QString& source, const CrawlerResult& result);

private:
    CrawlerResult searchArXiv(const QString& keyword);
    CrawlerResult searchSemanticScholar(const QString& keyword);
    CrawlerResult searchPubMed(const QString& keyword);

    bool isDuplicate(const Paper& paper);
    void deduplicatePapers(QList<Paper>& papers);

    CrawlerConfig config_;
    CrawlerStatus status_{CrawlerStatus::Idle};
    LocalDatabase* localDb_{nullptr};
    QThreadPool* threadPool_{nullptr};
};
