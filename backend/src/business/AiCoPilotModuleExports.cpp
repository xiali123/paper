#include "business/AiCoPilotModule.hpp"
#include "data/IDatabase.hpp"
#include "core/ServiceContainer.hpp"

using namespace PaperCrawler;

// 模块工厂函数
extern "C" {
    PAPERCRAWLER_MODULE_EXPORT const char* getModuleVersion() {
        return "1.0.0";
    }

    PAPERCRAWLER_MODULE_EXPORT const char* getModuleName() {
        return "AiCoPilot";
    }

    PAPERCRAWLER_MODULE_EXPORT void* createModule() {
        // 从ServiceContainer解析IDatabase
        auto database = Services::resolve<IDatabase>();
        if (!database) {
            return nullptr;
        }

        return static_cast<void*>(new AiCoPilotModule(database));
    }

    PAPERCRAWLER_MODULE_EXPORT void destroyModule(void* modulePtr) {
        delete static_cast<AiCoPilotModule*>(modulePtr);
    }

    PAPERCRAWLER_MODULE_EXPORT const char* getModuleDescription() {
        return "AI Research Co-Pilot with review, literature review, and planning assistance";
    }

    PAPERCRAWLER_MODULE_EXPORT int getModuleType() {
        return 2; // BUSINESS module type
    }

    PAPERCRAWLER_MODULE_EXPORT const char* getRoutePrefix() {
        return "/api/ai-co-pilot";
    }
}
