// 正确的executeRequest函数实现
std::string TemplateCrawlerModule::executeRequest(
    const CrawlerTemplate& tmpl,
    const std::string& url,
    const std::map<std::string, std::string>& params) {

    auto logger = spdlog::get("TemplateCrawler");
    if (logger) {
        logger->debug("Executing request: " + url);
    }

    // 使用HTTP客户端发送请求（简化版本）
    Network::HttpClientResponse httpResponse = httpClient_->get(url);

    if (!httpResponse.isSuccess()) {
        throw std::runtime_error("HTTP request failed with status: " +
            std::to_string(httpResponse.statusCode));
    }

    // TODO: 实现POST方法支持
    // TODO: 实现自定义headers支持
    // TODO: 实现timeout配置

    return httpResponse.body;
}
