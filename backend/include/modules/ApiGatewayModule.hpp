#pragma once

#include "framework/IModule.hpp"
#include "framework/Router.hpp"
#include <memory>

namespace PaperCrawler {

/**
 * @brief API网关模块
 *
 * 负责HTTP服务器和请求路由
 * 自动发现和注册BUSINESS类型的模块路由
 */
class ApiGatewayModule : public IModule {
public:
    ApiGatewayModule();
    ~ApiGatewayModule() override;

    // IModule接口实现
    std::string getName() const override { return "ApiGatewayModule"; }
    std::string getVersion() const override { return "1.0.0"; }
    std::string getDescription() const override;
    ModuleType getModuleType() const override { return ModuleType::SERVER; }
    std::string getRoutePrefix() const override { return ""; }

    bool initialize() override;
    bool start() override;
    bool stop() override;
    void cleanup() override;

    /**
     * @brief 设置服务器端口
     */
    void setPort(int port) { port_ = port; }

    /**
     * @brief 自动注册BUSINESS模块的路由
     */
    void autoRegisterBusinessModules(const std::vector<IModule*>& modules);

private:
    class Impl;
    std::unique_ptr<Impl> impl_;

    int port_ = 8080;
};

} // namespace PaperCrawler
