#pragma once

namespace PaperCrawler {
namespace HTTP {

constexpr int OK = 200;
constexpr int CREATED = 201;
constexpr int NO_CONTENT = 204;
constexpr int BAD_REQUEST = 400;
constexpr int UNAUTHORIZED = 401;
constexpr int FORBIDDEN = 403;
constexpr int NOT_FOUND = 404;
constexpr int CONFLICT = 409;
constexpr int INTERNAL_ERROR = 500;
constexpr int NOT_IMPLEMENTED = 501;
constexpr int SERVICE_UNAVAILABLE = 503;

} // namespace HTTP
} // namespace PaperCrawler
