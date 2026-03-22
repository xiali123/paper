#include "core/Config.hpp"
#include <fstream>
#include <sstream>

namespace PaperCrawler {

Config& Config::getInstance() {
    static Config instance;
    return instance;
}

void Config::load(const std::string& path) {
    std::lock_guard<std::mutex> lock(mutex_);

    try {
        std::ifstream file(path);
        if (!file.is_open()) {
            throw ConfigException("Cannot open config file: " + path);
        }

        file >> config_;
    } catch (const nlohmann::json::parse_error& e) {
        throw ConfigException("JSON parse error: " + std::string(e.what()));
    } catch (const std::exception& e) {
        throw ConfigException("Failed to load config: " + std::string(e.what()));
    }
}

std::string Config::get(const std::string& key, const std::string& defaultValue) {
    std::lock_guard<std::mutex> lock(mutex_);

    const nlohmann::json* value = getByPath(key);
    if (value && value->is_string()) {
        return value->get<std::string>();
    }
    return defaultValue;
}

int Config::getInt(const std::string& key, int defaultValue) {
    std::lock_guard<std::mutex> lock(mutex_);

    const nlohmann::json* value = getByPath(key);
    if (value && value->is_number_integer()) {
        return value->get<int>();
    }
    return defaultValue;
}

bool Config::getBool(const std::string& key, bool defaultValue) {
    std::lock_guard<std::mutex> lock(mutex_);

    const nlohmann::json* value = getByPath(key);
    if (value && value->is_boolean()) {
        return value->get<bool>();
    }
    return defaultValue;
}

double Config::getDouble(const std::string& key, double defaultValue) {
    std::lock_guard<std::mutex> lock(mutex_);

    const nlohmann::json* value = getByPath(key);
    if (value && value->is_number()) {
        return value->get<double>();
    }
    return defaultValue;
}

bool Config::has(const std::string& key) const {
    return getByPath(key) != nullptr;
}

void Config::set(const std::string& key, const std::string& value) {
    std::lock_guard<std::mutex> lock(mutex_);

    std::vector<std::string> parts;
    std::stringstream ss(key);
    std::string part;

    while (std::getline(ss, part, '.')) {
        parts.push_back(part);
    }

    nlohmann::json* current = &config_;
    for (size_t i = 0; i < parts.size() - 1; ++i) {
        if (!current->contains(parts[i])) {
            (*current)[parts[i]] = nlohmann::json::object();
        }
        current = &(*current)[parts[i]];
    }

    (*current)[parts.back()] = value;
}

void Config::save(const std::string& path) {
    std::lock_guard<std::mutex> lock(mutex_);

    try {
        std::ofstream file(path);
        if (!file.is_open()) {
            throw ConfigException("Cannot open config file for writing: " + path);
        }

        file << config_.dump(4);  // Pretty print with 4 spaces
    } catch (const std::exception& e) {
        throw ConfigException("Failed to save config: " + std::string(e.what()));
    }
}

const nlohmann::json* Config::getByPath(const std::string& key) const {
    std::vector<std::string> parts;
    std::stringstream ss(key);
    std::string part;

    while (std::getline(ss, part, '.')) {
        parts.push_back(part);
    }

    const nlohmann::json* current = &config_;
    for (const auto& p : parts) {
        if (!current->contains(p)) {
            return nullptr;
        }
        current = &(*current)[p];
    }

    return current;
}

} // namespace PaperCrawler
