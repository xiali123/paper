#pragma once

#include "core/Config.hpp"
#include "core/Logger.hpp"
#include <string>
#include <memory>

namespace PaperCrawler {

/**
 * @brief Main application class
 *
 * Manages application lifecycle and initialization
 */
class Application {
public:
    /**
     * @brief Constructor
     */
    Application() = default;

    /**
     * @brief Destructor
     */
    ~Application() = default;

    /**
     * @brief Initialize application
     * @param configFile Path to configuration file
     */
    void initialize(const std::string& configFile = "config/config.json");

    /**
     * @brief Run the application with given keyword
     * @param keyword Search keyword
     * @return Exit code (0 for success, non-zero for error)
     */
    int run(const std::string& keyword);

    /**
     * @brief Shutdown application
     */
    void shutdown();

private:
    bool initialized_{false};
};

} // namespace PaperCrawler
