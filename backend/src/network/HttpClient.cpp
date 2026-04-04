/**
 * @file HttpClient.cpp
 * @brief Implementation of HTTP client using libcurl
 */

#ifdef _WIN32
    // Undefine Windows macros that conflict with enum values
    #ifdef GET
    #undef GET
    #endif
    #ifdef POST
    #undef POST
    #endif
    #ifdef PUT
    #undef PUT
    #endif
    #ifdef DELETE
    #undef DELETE
    #endif
#endif

#include "network/HttpClient.hpp"
#include <curl/curl.h>
#include <sstream>
#include <cstring>
#include <algorithm>
#include <cctype>

namespace PaperCrawler::Network {

/**
 * @brief Implementation details (PIMPL pattern)
 */
class HttpClient::Impl {
public:
    CURL* curl;
    std::map<std::string, std::string> defaultHeaders;
    long timeout;
    bool verbose;

    Impl() : curl(nullptr), timeout(30), verbose(false) {
        curl = curl_easy_init();
        if (!curl) {
            throw std::runtime_error("Failed to initialize libcurl");
        }

        // Set default options
        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
        curl_easy_setopt(curl, CURLOPT_MAXREDIRS, 5L);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 1L);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 2L);
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, timeout);
    }

    ~Impl() {
        if (curl) {
            curl_easy_cleanup(curl);
        }
    }

    /**
     * @brief Callback for writing response data
     */
    static size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp) {
        size_t totalSize = size * nmemb;
        std::string* response = static_cast<std::string*>(userp);
        response->append(static_cast<char*>(contents), totalSize);
        return totalSize;
    }

    /**
     * @brief Callback for writing headers
     */
    static size_t HeaderCallback(char* buffer, size_t size, size_t nitems, void* userdata) {
        size_t totalSize = size * nitems;
        std::map<std::string, std::string>* headers =
            static_cast<std::map<std::string, std::string>*>(userdata);

        std::string header(buffer, totalSize);

        // Split header line by ":"
        size_t colonPos = header.find(':');
        if (colonPos != std::string::npos) {
            std::string key = header.substr(0, colonPos);
            std::string value = header.substr(colonPos + 1);

            // Trim whitespace
            size_t keyEnd = key.find_last_not_of(" \t\r\n");
            if (keyEnd != std::string::npos) {
                key = key.substr(0, keyEnd + 1);
            }

            size_t valueStart = value.find_first_not_of(" \t\r\n");
            size_t valueEnd = value.find_last_not_of(" \t\r\n");
            if (valueStart != std::string::npos && valueEnd != std::string::npos) {
                value = value.substr(valueStart, valueEnd - valueStart + 1);
            }

            // Convert key to lowercase for case-insensitive lookup
            std::transform(key.begin(), key.end(), key.begin(), ::tolower);
            (*headers)[key] = value;
        }

        return totalSize;
    }

    HttpClientResponse performRequest(HttpMethod method,
                                      const std::string& url,
                                      const std::map<std::string, std::string>& headers,
                                      const std::string& body) {
        HttpClientResponse response;

        if (!curl) {
            response.errorMessage = "CURL not initialized";
            return response;
        }

        // Reset all options
        curl_easy_reset(curl);

        // Set common options
        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
        curl_easy_setopt(curl, CURLOPT_MAXREDIRS, 5L);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 1L);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 2L);
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, timeout);
        curl_easy_setopt(curl, CURLOPT_VERBOSE, verbose ? 1L : 0L);

        // Set URL
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());

        // Set method
        switch (method) {
            case HttpMethod::GET:
                curl_easy_setopt(curl, CURLOPT_HTTPGET, 1L);
                break;
            case HttpMethod::POST:
                curl_easy_setopt(curl, CURLOPT_POST, 1L);
                break;
            case HttpMethod::PUT:
                curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "PUT");
                break;
            case HttpMethod::DEL:
                curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "DELETE");
                break;
            case HttpMethod::PATCH:
                curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "PATCH");
                break;
        }

        // Set body for POST/PUT/PATCH
        if (!body.empty() &&
            (method == HttpMethod::POST || method == HttpMethod::PUT ||
             method == HttpMethod::PATCH)) {
            curl_easy_setopt(curl, CURLOPT_POSTFIELDS, body.c_str());
        }

        // Set headers
        struct curl_slist* headerList = nullptr;

        // Add default headers
        for (const auto& [key, value] : defaultHeaders) {
            std::string headerLine = key + ": " + value;
            headerList = curl_slist_append(headerList, headerLine.c_str());
        }

        // Add custom headers
        for (const auto& [key, value] : headers) {
            std::string headerLine = key + ": " + value;
            headerList = curl_slist_append(headerList, headerLine.c_str());
        }

        if (headerList) {
            curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headerList);
        }

        // Set response callbacks
        std::string responseBody;
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &responseBody);

        std::map<std::string, std::string> responseHeaders;
        curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, HeaderCallback);
        curl_easy_setopt(curl, CURLOPT_HEADERDATA, &responseHeaders);

        // Perform request
        CURLcode res = curl_easy_perform(curl);

        if (res != CURLE_OK) {
            response.errorMessage = curl_easy_strerror(res);
            response.statusCode = 0;
        } else {
            long httpCode = 0;
            curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &httpCode);
            response.statusCode = static_cast<int>(httpCode);
            response.body = responseBody;
            response.headers = responseHeaders;
        }

        // Cleanup header list
        if (headerList) {
            curl_slist_free_all(headerList);
        }

        return response;
    }
};

HttpClient::HttpClient() : pImpl_(new Impl()) {
}

HttpClient::~HttpClient() {
    delete pImpl_;
}

HttpClientResponse HttpClient::get(const std::string& url) {
    return request(HttpMethod::GET, url);
}

HttpClientResponse HttpClient::post(const std::string& url, const std::string& jsonBody) {
    std::map<std::string, std::string> headers;
    headers["Content-Type"] = "application/json";
    return request(HttpMethod::POST, url, headers, jsonBody);
}

HttpClientResponse HttpClient::postForm(const std::string& url,
                                        const std::map<std::string, std::string>& formData) {
    // Build form-encoded body
    std::string formBody;
    for (auto it = formData.begin(); it != formData.end(); ++it) {
        if (it != formData.begin()) {
            formBody += "&";
        }
        formBody += it->first + "=" + it->second;
    }

    std::map<std::string, std::string> headers;
    headers["Content-Type"] = "application/x-www-form-urlencoded";

    return request(HttpMethod::POST, url, headers, formBody);
}

HttpClientResponse HttpClient::put(const std::string& url, const std::string& body) {
    std::map<std::string, std::string> headers;
    headers["Content-Type"] = "application/json";
    return request(HttpMethod::PUT, url, headers, body);
}

HttpClientResponse HttpClient::del(const std::string& url) {
    return request(HttpMethod::DEL, url);
}

HttpClientResponse HttpClient::request(HttpMethod method,
                                      const std::string& url,
                                      const std::map<std::string, std::string>& headers,
                                      const std::string& body) {
    return pImpl_->performRequest(method, url, headers, body);
}

void HttpClient::setDefaultHeader(const std::string& key, const std::string& value) {
    pImpl_->defaultHeaders[key] = value;
}

void HttpClient::setTimeout(long seconds) {
    pImpl_->timeout = seconds;
}

void HttpClient::setVerbose(bool verbose) {
    pImpl_->verbose = verbose;
}

} // namespace PaperCrawler::Network
