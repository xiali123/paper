#include <iostream>
#include <memory>
#include "data/DatabaseModule.hpp"

int main() {
    std::cout << "Database module test..." << std::endl;
    
    try {
        auto dbModule = std::make_shared<PaperCrawler::DatabaseModule>();
        std::cout << "DatabaseModule created" << std::endl;
        
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Exception: " << e.what() << std::endl;
        return 1;
    }
}
