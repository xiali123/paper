// PaperCrawler API Server - Fixed version for frontend compatibility
// This patch fixes the JSON response format to match frontend expectations

// ============= PROBLEM =============
// Backend returns:           Frontend expects:
// "journal_full": "..."       "journal": {"full": "...", "short": "..."}
// "journal_short": "..."
// "doi_url": "..."            "urls": {"doi": "...", "journal": "..."}
// "journal_url": "..."
// "author": "..."              "authors": "..."

// ============= SOLUTION =============
// Replace the paperToJson function in api_server.cpp with this fixed version:

std::string paperToJsonFixed(const Paper& paper) {
    std::ostringstream json;
    json << "{\n";
    json << "  \"id\": " << paper.getId() << ",\n";
    json << "  \"title\": \"" << escapeJsonString(paper.getTitle()) << "\",\n";

    // Changed: nested journal object (frontend expects this)
    json << "  \"journal\": {\n";
    json << "    \"full\": \"" << escapeJsonString(paper.getJournalFull()) << "\",\n";
    json << "    \"short\": \"" << escapeJsonString(paper.getJournalShort()) << "\"\n";
    json << "  },\n";

    json << "  \"year\": \"" << escapeJsonString(paper.getYear()) << "\",\n";

    // Changed: "authors" instead of "author" (frontend expects this)
    json << "  \"authors\": \"" << escapeJsonString(paper.getAuthor()) << "\",\n";

    // Changed: nested urls object (frontend expects this)
    json << "  \"urls\": {\n";
    std::string doi = paper.getDoiUrl();
    std::string journalUrl = paper.getJournalUrl();
    if (!doi.empty()) {
        json << "    \"doi\": \"" << escapeJsonString(doi) << "\",\n";
    }
    if (!journalUrl.empty()) {
        json << "    \"journal\": \"" << escapeJsonString(journalUrl) << "\",\n";
    }
    // Remove trailing comma if needed
    std::string urlsStr = json.str();
    if (urlsStr.back() == ',') {
        urlsStr.pop_back();
        // Reset json to build without trailing comma
        json.str("");
        json.clear();
        json << urlsStr;
    }
    json << "  },\n";

    json << "  \"level\": \"" << escapeJsonString(paper.getLevel()) << "\"\n";
    json << "}";
    return json.str();
}

// Also, in handleSearch function, change the response to match frontend format:
// The frontend expects { papers, total, keyword, duration } not wrapped in { success, data }

std::string handleSearchFixed(const std::map<std::string, std::string>& params) {
    // ... (existing validation and search logic) ...

    // Build response in frontend format (without success/data wrapper)
    std::ostringstream json;
    json << "{\n";
    json << "  \"papers\": [\n";
    for (size_t i = 0; i < filteredPapers.size(); ++i) {
        json << "    " << paperToJsonFixed(filteredPapers[i]);
        if (i < filteredPapers.size() - 1) json << ",";
        json << "\n";
    }
    json << "  ],\n";
    json << "  \"total\": " << totalCount << ",\n";
    json << "  \"keyword\": \"" << escapeJsonString(keyword) << "\",\n";
    json << "  \"duration\": " << std::fixed << std::setprecision(2) << duration << "\n";
    json << "}";

    return buildJsonResponse(json.str());
}

// Instructions:
// 1. Replace paperToJson() with paperToJsonFixed() in api_server.cpp
// 2. Replace handleSearch() response building with handleSearchFixed()
// 3. Recompile the backend
// 4. Test with both frontend and desktop client
