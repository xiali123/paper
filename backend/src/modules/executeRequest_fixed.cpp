std::string TemplateCrawlerModule::executeRequest(
    const CrawlerTemplate& tmpl,
    const std::string& url,
    const std::map<std::string, std::string>& params) {

    auto logger = spdlog::get("TemplateCrawler");
    if (logger) {
        logger->debug("Executing request: " + url);
    }

    // 使用HTTP客户端发送请求
    Network::HttpClientResponse httpResponse = httpClient_->get(url);

    if (!httpResponse.isSuccess()) {
        throw std::runtime_error("HTTP request failed with status: " +
            std::to_string(httpResponse.statusCode));
    }

    return httpResponse.body;

    // TODO: 支持自定义headers和timeout
    // TODO: 支持POST方法
}
