#include "parser/HtmlParser.hpp"
#include <gumbo.h>
#include <regex>
#include <algorithm>
#include <cctype>

namespace PaperCrawler {

// ============================================================================
// GumboInitializer Implementation
// ============================================================================

GumboInitializer::GumboInitializer() {
    // No explicit initialization needed for gumbo
}

GumboInitializer::~GumboInitializer() {
    // No cleanup needed for gumbo (it's a pure C library)
}

// ============================================================================
// Helper functions for HTML traversal
// ============================================================================

namespace {
    std::string extractTextFromNode(GumboNode* node) {
        if (!node) return "";

        if (node->type == GUMBO_NODE_TEXT) {
            return std::string(node->v.text.text);
        } else if (node->type == GUMBO_NODE_ELEMENT &&
                   node->v.element.tag != GUMBO_TAG_SCRIPT &&
                   node->v.element.tag != GUMBO_TAG_STYLE) {
            std::string result;
            GumboVector* children = &node->v.element.children;
            for (unsigned int i = 0; i < children->length; ++i) {
                const std::string text = extractTextFromNode(static_cast<GumboNode*>(children->data[i]));
                if (i > 0 && !text.empty()) result += " ";
                result += text;
            }
            return result;
        }
        return "";
    }

    void cleanString(std::string& str) {
        // Remove leading/trailing whitespace
        str.erase(0, str.find_first_not_of(" \t\n\r"));
        str.erase(str.find_last_not_of(" \t\n\r") + 1);

        // Replace multiple spaces with single space
        std::regex multiSpace("\\s+");
        str = std::regex_replace(str, multiSpace, " ");
    }
}

// ============================================================================
// HtmlParser Implementation
// ============================================================================

std::string HtmlParser::extract(const std::string& html, const std::string& pattern) {
    try {
        std::regex re(pattern, regexFlags_);
        std::smatch match;

        if (std::regex_search(html, match, re) && match.size() > 1) {
            return match[1].str();
        }
    } catch (const std::regex_error& e) {
        throw ParseException("Regex error: " + std::string(e.what()));
    }

    return "";
}

std::vector<std::string> HtmlParser::extractAll(const std::string& html, const std::string& pattern) {
    std::vector<std::string> results;

    try {
        std::regex re(pattern, regexFlags_);
        std::sregex_iterator begin(html.begin(), html.end(), re);
        std::sregex_iterator end;

        for (std::sregex_iterator i = begin; i != end; ++i) {
            std::smatch match = *i;
            if (match.size() > 1) {
                results.push_back(match[1].str());
            }
        }
    } catch (const std::regex_error& e) {
        throw ParseException("Regex error: " + std::string(e.what()));
    }

    return results;
}

std::string HtmlParser::cleanText(const std::string& text) {
    std::string result = text;

    // Remove HTML entities
    std::regex entityRegex("&(#?[xX]?[0-9a-fA-F]+|[a-zA-Z]+);");
    // Basic HTML entity decoding
    result = std::regex_replace(result, std::regex("&amp;"), "&");
    result = std::regex_replace(result, std::regex("&lt;"), "<");
    result = std::regex_replace(result, std::regex("&gt;"), ">");
    result = std::regex_replace(result, std::regex("&quot;"), "\"");
    result = std::regex_replace(result, std::regex("&apos;"), "'");
    result = std::regex_replace(result, std::regex("&nbsp;"), " ");

    // Clean whitespace
    cleanString(result);

    return result;
}

std::string HtmlParser::stripTags(const std::string& html) {
    auto gumboOutput = parse(html);
    std::string text = extractTextFromNode(gumboOutput.get()->root);
    return cleanText(text);
}

std::string HtmlParser::extractAttr(const std::string& html, const std::string& tag, const std::string& attr) {
    // Build regex pattern for attribute extraction
    std::string pattern = "<" + tag + "[^>]*" + attr + "\\s*=\\s*[\"']([^\"']*)[\"']";
    return extract(html, pattern);
}

std::string HtmlParser::extractElementText(const std::string& html, const std::string& selector) {
    // Simplified CSS selector (only class and id supported)
    std::string pattern;

    if (selector[0] == '.') {
        // Class selector
        std::string className = selector.substr(1);
        pattern = "class\\s*=\\s*[\"'][^\"']*\\b" + className + "\\b[^\"']*[\"'][^>]*>([^<]*)";
    } else if (selector[0] == '#') {
        // ID selector
        std::string id = selector.substr(1);
        pattern = "id\\s*=\\s*[\"']" + id + "[\"'][^>]*>([^<]*)";
    } else {
        // Tag selector
        pattern = "<" + selector + "[^>]*>([^<]*)</" + selector + ">";
    }

    std::string result = extract(html, pattern);
    return cleanText(result);
}

std::unique_ptr<GumboOutput, void(*)(GumboOutput*)> HtmlParser::parse(const std::string& html) {
    GumboOutput* output = gumbo_parse(html.c_str());
    return std::unique_ptr<GumboOutput, void(*)(GumboOutput*)>(
        output,
        [](GumboOutput* ptr) { gumbo_destroy_output(&kGumboDefaultOptions, ptr); }
    );
}

} // namespace PaperCrawler
