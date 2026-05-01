#include "features/ai/EmbeddingGenerator.hpp"
#include "network/HttpClient.hpp"
#include <spdlog/spdlog.h>
#include <sstream>
#include <cmath>
#include <algorithm>
#include <cctype>
#include <locale>

namespace PaperCrawler {

EmbeddingGenerator::EmbeddingGenerator() = default;

void EmbeddingGenerator::setProvider(const std::string& provider) {
    if (provider == "local" || provider == "openai") {
        provider_ = provider;
    } else {
        spdlog::warn("[EmbeddingGenerator] Unknown provider '{}', using 'local'", provider);
        provider_ = "local";
    }
}

void EmbeddingGenerator::setApiKey(const std::string& key) {
    apiKey_ = key;
}

void EmbeddingGenerator::setModel(const std::string& model) {
    model_ = model;
}

void EmbeddingGenerator::setDimension(int dim) {
    if (dim > 0 && dim <= 4096) {
        dimensions_ = dim;
    } else {
        spdlog::warn("[EmbeddingGenerator] Invalid dimension {}, using default 384", dim);
        dimensions_ = 384;
    }
}

std::vector<float> EmbeddingGenerator::generate(const std::string& text) {
    if (text.empty()) {
        return std::vector<float>(dimensions_, 0.0f);
    }

    if (provider_ == "openai" && !apiKey_.empty()) {
        try {
            return generateOpenAI(text);
        } catch (const std::exception& e) {
            spdlog::warn("[EmbeddingGenerator] OpenAI API failed, falling back to local: {}", e.what());
            return generateLocalEmbedding(text, dimensions_);
        }
    }

    return generateLocalEmbedding(text, dimensions_);
}

std::vector<std::vector<float>> EmbeddingGenerator::generateBatch(const std::vector<std::string>& texts) {
    std::vector<std::vector<float>> results;
    results.reserve(texts.size());
    for (const auto& text : texts) {
        results.push_back(generate(text));
    }
    return results;
}

// --- Local embedding generation (FNV-1a hash based TF approach) ---

std::vector<float> EmbeddingGenerator::generateLocalEmbedding(const std::string& text, int dimensions) {
    std::vector<float> embedding(static_cast<size_t>(dimensions), 0.0f);

    if (text.empty()) {
        return embedding;
    }

    // Tokenize: split by whitespace and punctuation, lowercase
    std::vector<std::string> tokens;
    std::string currentToken;

    for (char c : text) {
        if (std::isalnum(static_cast<unsigned char>(c))) {
            currentToken += static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
        } else {
            if (!currentToken.empty()) {
                tokens.push_back(currentToken);
                currentToken.clear();
            }
        }
    }
    if (!currentToken.empty()) {
        tokens.push_back(currentToken);
    }

    if (tokens.empty()) {
        return embedding;
    }

    // Hash each token into multiple dimensions using FNV-1a with different seeds
    // This creates a sparse but deterministic embedding
    for (size_t t = 0; t < tokens.size(); ++t) {
        const std::string& token = tokens[t];

        // Primary hash: maps token to a dimension
        uint32_t primaryHash = fnv1aHash(token);
        size_t primaryDim = primaryHash % static_cast<size_t>(dimensions);

        // Secondary hash: maps token to another dimension (bigram-like)
        std::string bigram = (t > 0) ? (tokens[t - 1] + "_" + token) : ("^_" + token);
        uint32_t secondaryHash = fnv1aHash(bigram);
        size_t secondaryDim = secondaryHash % static_cast<size_t>(dimensions);

        // Tertiary: character n-gram contribution
        if (token.size() >= 3) {
            for (size_t i = 0; i + 2 < token.size(); i += 2) {
                std::string trigram = token.substr(i, 3);
                uint32_t triHash = fnv1aHash(trigram);
                size_t triDim = triHash % static_cast<size_t>(dimensions);
                embedding[triDim] += hashToFloat(triHash) * 0.3f;
            }
        }

        // Accumulate at primary and secondary dimensions
        embedding[primaryDim] += hashToFloat(primaryHash);
        embedding[secondaryDim] += hashToFloat(secondaryHash) * 0.7f;
    }

    // Normalize vector (L2 normalization for cosine similarity)
    float norm = 0.0f;
    for (float v : embedding) {
        norm += v * v;
    }
    norm = std::sqrt(norm);

    if (norm > 1e-8f) {
        for (float& v : embedding) {
            v /= norm;
        }
    }

    return embedding;
}

uint32_t EmbeddingGenerator::fnv1aHash(const std::string& str) {
    // FNV-1a 32-bit hash
    uint32_t hash = 2166136261u;
    for (char c : str) {
        hash ^= static_cast<uint32_t>(static_cast<unsigned char>(c));
        hash *= 16777619u;
    }
    return hash;
}

float EmbeddingGenerator::hashToFloat(uint32_t hash) {
    // Map hash to range [-1.0, 1.0] using hash bits
    // Use the upper bits for better distribution
    return static_cast<float>(static_cast<int32_t>(hash)) / static_cast<float>(0x7FFFFFFF);
}

// --- OpenAI embedding API ---

std::vector<float> EmbeddingGenerator::generateOpenAI(const std::string& text) {
    Network::HttpClient client;
    client.setTimeout(30);

    // Build JSON request body
    // Escape text for JSON
    std::string escapedText;
    for (char c : text) {
        switch (c) {
            case '"':  escapedText += "\\\""; break;
            case '\\': escapedText += "\\\\"; break;
            case '\n': escapedText += "\\n"; break;
            case '\r': escapedText += "\\r"; break;
            case '\t': escapedText += "\\t"; break;
            default:   escapedText += c; break;
        }
    }

    std::ostringstream jsonBody;
    jsonBody << "{";
    jsonBody << "\"model\":\"" << model_ << "\",";
    jsonBody << "\"input\":\"" << escapedText << "\"";
    if (dimensions_ != 384) {
        jsonBody << ",\"dimensions\":" << dimensions_;
    }
    jsonBody << "}";

    // Set authorization header
    client.setDefaultHeader("Authorization", "Bearer " + apiKey_);
    client.setDefaultHeader("Content-Type", "application/json");

    auto response = client.post("https://api.openai.com/v1/embeddings", jsonBody.str());

    if (response.statusCode != 200) {
        throw std::runtime_error("OpenAI API error: HTTP " + std::to_string(response.statusCode) +
                                  " - " + response.body);
    }

    // Parse embedding from JSON response
    // Response format: {"data":[{"embedding":[0.1,0.2,...],"index":0}]}
    std::vector<float> embedding;

    // Find the embedding array in the response
    std::string marker = "\"embedding\":[";
    size_t start = response.body.find(marker);
    if (start == std::string::npos) {
        throw std::runtime_error("Failed to parse OpenAI embedding response: embedding field not found");
    }

    start += marker.length();
    size_t end = response.body.find("]", start);
    if (end == std::string::npos) {
        throw std::runtime_error("Failed to parse OpenAI embedding response: array end not found");
    }

    std::string arrayContent = response.body.substr(start, end - start);

    // Parse comma-separated float values
    std::istringstream iss(arrayContent);
    std::string token;
    while (std::getline(iss, token, ',')) {
        try {
            // Trim whitespace
            size_t first = token.find_first_not_of(" \t\n\r");
            size_t last = token.find_last_not_of(" \t\n\r");
            if (first != std::string::npos && last != std::string::npos) {
                token = token.substr(first, last - first + 1);
            }
            if (!token.empty()) {
                embedding.push_back(std::stof(token));
            }
        } catch (...) {
            // Skip malformed values
        }
    }

    if (embedding.empty()) {
        throw std::runtime_error("OpenAI returned empty embedding vector");
    }

    spdlog::debug("[EmbeddingGenerator] OpenAI embedding generated: {} dimensions", embedding.size());
    return embedding;
}

} // namespace PaperCrawler
