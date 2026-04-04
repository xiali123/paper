/**
 * @file SimpleHttpClient.cpp
 * @brief Simple HTTP client using Windows WinINet
 */

#include "network/HttpClient.hpp"
#include <windows.h>
#include <wininet.h>
#include <sstream>
#include <algorithm>
#include <cctype>

#pragma comment(lib, "wininet.lib")

namespace PaperCrawler::Network {

class HttpClient::Impl {
public:
    HINTERNET hInternet;
    std::map<std::string, std::string> defaultHeaders;
    long timeout;
    bool verbose;

    Impl() : hInternet(nullptr), timeout(30), verbose(false) {
        hInternet = InternetOpenA("PaperCrawler/1.0",
                                  INTERNET_OPEN_TYPE_PRECONFIG,
                                  nullptr,
                                  nullptr,
                                  0);
        if (!hInternet) {
            throw std::runtime_error("Failed to initialize WinINet");
        }

        // Set timeout
        InternetSetOptionA(hInternet, INTERNET_OPTION_CONNECT_TIMEOUT, &timeout, sizeof(timeout));
        InternetSetOptionA(hInternet, INTERNET_OPTION_RECEIVE_TIMEOUT, &timeout, sizeof(timeout));
    }

    ~Impl() {
        if (hInternet) {
            InternetCloseHandle(hInternet);
        }
    }

    static std::string urlEncode(const std::string& str) {
        std::string encoded;
        for (char c : str) {
            if (std::isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~') {
                encoded += c;
            } else {
                char buf[4];
                snprintf(buf, sizeof(buf), "%%%02X", (unsigned char)c);
                encoded += buf;
            }
        }
        return encoded;
    }

    HttpClientResponse performRequest(HttpMethod method,
                                      const std::string& url,
                                      const std::map<std::string, std::string>& headers,
                                      const std::string& body) {
        HttpClientResponse response;

        if (!hInternet) {
            response.errorMessage = "WinINet not initialized";
            return response;
        }

        // Parse URL
        std::string server, object, urlPath;
        bool secure = false;

        if (url.find("https://") == 0) {
            server = url.substr(8);
            secure = true;
        } else if (url.find("http://") == 0) {
            server = url.substr(7);
        } else {
            response.errorMessage = "Invalid URL protocol";
            return response;
        }

        // Split server and path
        size_t slashPos = server.find('/');
        if (slashPos != std::string::npos) {
            object = server.substr(slashPos);
            server = server.substr(0, slashPos);
        } else {
            object = "/";
        }

        // Remove query string from object for the request
        urlPath = object;

        // Open connection
        HINTERNET hConnect = InternetConnectA(hInternet,
                                               server.c_str(),
                                               secure ? INTERNET_DEFAULT_HTTPS_PORT : INTERNET_DEFAULT_HTTP_PORT,
                                               nullptr,
                                               nullptr,
                                               INTERNET_SERVICE_HTTP,
                                               0,
                                               0);

        if (!hConnect) {
            response.errorMessage = "Failed to connect to server";
            return response;
        }

        // Build request type
        const char* requestType = "GET";
        switch (method) {
            case HttpMethod::GET: requestType = "GET"; break;
            case HttpMethod::POST: requestType = "POST"; break;
            case HttpMethod::PUT: requestType = "PUT"; break;
            case HttpMethod::DEL: requestType = "DELETE"; break;
            case HttpMethod::PATCH: requestType = "PATCH"; break;
        }

        // Open request
        HINTERNET hRequest = HttpOpenRequestA(hConnect,
                                              requestType,
                                              object.c_str(),
                                              nullptr,
                                              nullptr,
                                              nullptr,
                                              INTERNET_FLAG_RELOAD | (secure ? INTERNET_FLAG_SECURE : 0),
                                              0);

        if (!hRequest) {
            InternetCloseHandle(hConnect);
            response.errorMessage = "Failed to open request";
            return response;
        }

        // Add headers
        std::string headersStr;
        for (const auto& [key, value] : defaultHeaders) {
            headersStr += key + ": " + value + "\r\n";
        }
        for (const auto& [key, value] : headers) {
            headersStr += key + ": " + value + "\r\n";
        }

        // Send request
        if (!HttpSendRequestA(hRequest,
                             headersStr.empty() ? nullptr : headersStr.c_str(),
                             headersStr.length(),
                             (LPVOID)(body.empty() ? nullptr : body.c_str()),
                             body.length())) {
            InternetCloseHandle(hRequest);
            InternetCloseHandle(hConnect);
            response.errorMessage = "Failed to send request";
            return response;
        }

        // Read response
        std::string responseBody;
        char buffer[4096];
        DWORD bytesRead = 0;

        while (InternetReadFile(hRequest, buffer, sizeof(buffer) - 1, &bytesRead) && bytesRead > 0) {
            buffer[bytesRead] = '\0';
            responseBody += buffer;
        }

        // Get status code
        DWORD statusCode = 0;
        DWORD statusSize = sizeof(statusCode);
        HttpQueryInfo(hRequest, HTTP_QUERY_STATUS_CODE | HTTP_QUERY_FLAG_NUMBER, &statusCode, &statusSize, nullptr);

        response.statusCode = (int)statusCode;
        response.body = responseBody;

        // Get headers
        char headersBuffer[4096];
        DWORD headersSize = sizeof(headersBuffer);
        if (HttpQueryInfo(hRequest, HTTP_QUERY_RAW_HEADERS_CRLF, headersBuffer, &headersSize, nullptr)) {
            // Parse headers if needed
        }

        InternetCloseHandle(hRequest);
        InternetCloseHandle(hConnect);

        return response;
    }
};

// Keep existing HttpClient interface methods...
HttpClient::HttpClient() : pImpl_(new Impl()) {}
HttpClient::~HttpClient() { delete pImpl_; }

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
    std::string formBody;
    for (auto it = formData.begin(); it != formData.end(); ++it) {
        if (it != formData.begin()) formBody += "&";
        formBody += Impl::urlEncode(it->first) + "=" + Impl::urlEncode(it->second);
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
