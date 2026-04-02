#pragma once

#include <string>
#include <vector>
#include <map>
#include <optional>
#include <memory>

namespace PaperCrawler {

// 前向声明
struct CrawlerTemplate;
struct CrawledPaper;

/**
 * @brief 爬虫接口
 *
 * 定义爬虫的抽象能力，支持多种爬虫实现：
 * - TemplateCrawlerModule（模板爬虫）
 * - ApiCrawlerModule（API爬虫）
 * - BrowserCrawlerModule（浏览器爬虫）
 */
class ICrawler {
public:
    virtual ~ICrawler() = default;

    /**
     * @brief 使用模板爬取数据
     * @param templateId 模板ID
     * @param params 爬取参数
     * @return 爬取的论文列表
     */
    virtual std::vector<CrawledPaper> crawlWithTemplate(
        const std::string& templateId,
        const std::map<std::string, std::string>& params
    ) = 0;

    /**
     * @brief 验证模板
     * @param tmpl 模板配置
     * @return 验证结果
     */
    virtual bool validateTemplate(const CrawlerTemplate& tmpl) = 0;

    /**
     * @brief 测试模板
     * @param templateId 模板ID
     * @param params 测试参数
     * @return 测试结果
     */
    virtual std::optional<std::vector<CrawledPaper>> testTemplate(
        const std::string& templateId,
        const std::map<std::string, std::string>& params
    ) = 0;

    /**
     * @brief 获取模板列表
     * @param activeOnly 是否只返回启用的模板
     * @return 模板列表
     */
    virtual std::vector<CrawlerTemplate> listTemplates(bool activeOnly = true) = 0;

    /**
     * @brief 保存模板
     * @param tmpl 模板配置
     * @param createdBy 创建者ID
     * @return 是否成功
     */
    virtual bool saveTemplate(const CrawlerTemplate& tmpl, int createdBy) = 0;

    /**
     * @brief 删除模板
     * @param templateId 模板ID
     * @return 是否成功
     */
    virtual bool deleteTemplate(const std::string& templateId) = 0;
};

} // namespace PaperCrawler
