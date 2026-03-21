#include "network\HttpClient.hpp"
#include <curl/curl.h>
#include <cstring>
#include <thread>
#include <chrono>
#include <sstream>

namespace PaperCrawler {

// ============================================================================
// CurlGlobalInitializer Implementation
// ============================================================================

CurlGlobalInitializer::CurlGlobalInitializer() {
    curl_global_init(CURL_GLOBAL_ALL);
}

CurlGlobalInitializer::~CurlGlobalInitializer() {
    curl_global_cleanup();
}

namespace {
    // Global initializer (runs before main)
    CurlGlobalInitializer curlInit;
}

// ============================================================================
// Callback functions for libcurl
// ============================================================================

namespace {
    size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp) {
        size_t totalSize = size * nmemb;
        std::string* str = static_cast<std::string*>(userp);
        str->append(static_cast<char*>(contents), totalSize);
        return totalSize;
    }

    size_t HeaderCallback(char* buffer, size_t size, size_t nitems, void* userp) {
        size_t totalSize = size * nitems;
        std::string* headers = static_cast<std::string*>(userp);
        headers->append(buffer, totalSize);
        return totalSize;
    }
}

// ============================================================================
// HttpClient Implementation
// ============================================================================

HttpClient::HttpClient(int timeout)
    : defaultTimeout_(timeout), retryCount_(3), sslVerify_(true) {
    init();
}

HttpClient::~HttpClient() {
    if (curl_) {
        curl_easy_cleanup(curl_);
    }
}

void HttpClient::init() {
    curl_ = curl_easy_init();
    if (!curl_) {
        throw NetworkException("Failed to initialize CURL");
    }
}

HttpResponse HttpClient::get(const std::string& url, int timeout) {
    return performRequest(url, timeout > 0 ? timeout : defaultTimeout_);
}

HttpResponse HttpClient::post(const std::string& url, const std::string& data, int timeout) {
    return performRequest(url, timeout > 0 ? timeout : defaultTimeout_, data);
}

void HttpClient::setHeader(const std::string& key, const std::string& value) {
    headers_[key] = value;
}

void HttpClient::setUserAgent(const std::string& userAgent) {
    userAgent_ = userAgent;
}

void HttpClient::setProxy(const std::string& proxy) {
    proxy_ = proxy;
}

void HttpClient::setTimeout(int seconds) {
    defaultTimeout_ = seconds;
}

void HttpClient::setRetryCount(int retries) {
    retryCount_ = retries;
}

void HttpClient::setSSLVerify(bool enable) {
    sslVerify_ = enable;
}

HttpResponse HttpClient::performRequest(const std::string& url, int timeout,
                                       const std::string& postData) {
    if (!curl_) {
        throw NetworkException("CURL not initialized");
    }

    HttpResponse response;
    int retries = retryCount_;

    while (reries > 0) {
        std::string responseBody;
        std::string responseHeaders;

        // Reset CURL options
        curl_easy_reset(curl_);

        // Set URL
        curl_easy_setopt(curl_, CURLOPT_URL, url.c_str());

        // Set timeout
        curl_easy_setopt(curl_, CURLOPT_TIMEOUT, timeout);
        curl_easy_setopt(curl_, CURLOPT_CONNECTTIMEOUT, timeout / 2);

        // SSL verification
        curl_easy_setopt(curl_, CURLOPT_SSL_VERIFYPEER, sslVerify_ ? 1L : 0L);
        curl_easy_setopt(curl_, CURLOPT_SSL_VERIFYHOST, sslVerify_ ? 2L : 0L);

        // Set callbacks
        curl_easy_setopt(curl_, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl_, CURLOPT_WRITEDATA, &responseBody);
        curl_easy_setopt(curl_, CURLOPT_HEADERFUNCTION, HeaderCallback);
        curl_easy_setopt(curl_, CURLOPT_HEADERDATA, &responseHeaders);

        // Set headers
        struct curl_slist* headerList = nullptr;
        for (const auto& [key, value] : headers_) {
            std::string header = key + ": " + value;
            headerList = curl_slist_append(headerList, header.c_str());
        }

        // Set User-Agent
        if (!userAgent_.empty()) {
            curl_easy_setopt(curl_, CURLOPT_USERAGENT, userAgent_.c_str());
        }

        // Set proxy
        if (!proxy_.empty()) {
            curl_easy_setopt(curl_, CURLOPT_PROXY, proxy_.c_str());
        }

        // POST data
        if (!postData.empty()) {
            curl_easy_setopt(curl_, CURLOPT_POST, 1L);
            curl_easy_setopt(curl_, CURLOPT_POSTFIELDS, postData.c_str());
        }

        // Follow redirects
        curl_easy_setopt(curl_, CURLOPT_FOLLOWLOCATION, 1L);
        curl_easy_setopt(curl_, CURLOPT_MAXREDIRS, 5L);

        if (headerList) {
            curl_easy_setopt(curl_, CURLOPT_HTTPHEADER, headerList);
        }

        // Perform request
        CURLcode res = curl_easy_perform(curl_);

        if (headerList) {
            curl_slist_free_all(headerList);
        }

        if (res == CURLE_OK) {
            long httpCode = 0;
            curl_easy_getinfo(curl_, CURLINFO_RESPONSE_CODE, &httpCode);

            response.statusCode = static_cast<int>(httpCode);
            response.body = responseBody;
            response.headers = responseHeaders;
            response.success = (httpCode >= 200 && httpCode < 300);
            return response;
        }

        response.error = curl_easy_strerror(res);

        // Retry on timeout or connection errors
        if (res == CURLE_OPERATION_TIMEDOUT || res == CURLE_COULDNT_CONNECT) {
            retries--;
            if (retries > 0) {
                // Exponential backoff
                std::this_thread::sleep_for(std::chrono::milliseconds(100 * (retryCount_ - retries + 1)));
                continue;
            }
        }

        break;
    }

    throw NetworkException("HTTP request failed: " + response.error, url);
}

} // namespace PaperCrawler
