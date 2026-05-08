#pragma once

#include "core/ModuleBase.hpp"
#include "data/IDatabase.hpp"
#include <string>
#include <memory>

namespace PaperCrawler {

class AiCoPilotModule : public BusinessModuleBase {
public:
    AiCoPilotModule();
    explicit AiCoPilotModule(std::shared_ptr<IDatabase> database);
    ~AiCoPilotModule() override;

    std::string getName() const override { return "AiCoPilot"; }
    std::string getVersion() const override { return "1.0.0"; }
    std::string getDescription() const override {
        return "AI Research Co-Pilot with review, literature review, and planning assistance";
    }

private:
    std::shared_ptr<IDatabase> database_;
    void registerRoutes() override;
};

} // namespace PaperCrawler
