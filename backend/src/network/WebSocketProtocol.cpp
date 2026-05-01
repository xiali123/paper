#include "network/WebSocketProtocol.hpp"
#include <openssl/sha.h>
#include <sstream>
#include <algorithm>
#include <cstring>
#include <cstdint>

namespace PaperCrawler {

// ============================================================================
// Base64 encoder (standard alphabet, with padding)
// ============================================================================

std::string WebSocketProtocol::base64Encode(const unsigned char* data, size_t len) {
    static const char table[] =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

    std::string out;
    out.reserve(((len + 2) / 3) * 4);

    for (size_t i = 0; i < len; i += 3) {
        unsigned char b0 = data[i];
        unsigned char b1 = (i + 1 < len) ? data[i + 1] : 0;
        unsigned char b2 = (i + 2 < len) ? data[i + 2] : 0;

        out.push_back(table[b0 >> 2]);
        out.push_back(table[((b0 & 0x03) << 4) | (b1 >> 4)]);
        out.push_back((i + 1 < len) ? table[((b1 & 0x0F) << 2) | (b2 >> 6)] : '=');
        out.push_back((i + 2 < len) ? table[b2 & 0x3F] : '=');
    }

    return out;
}

// ============================================================================
// Handshake (RFC 6455 Section 4)
// ============================================================================

bool WebSocketProtocol::isWebSocketUpgrade(const std::string& httpRequest) {
    // Case-insensitive search for Upgrade: websocket and the method GET
    std::string lower;
    lower.reserve(httpRequest.size());
    for (char c : httpRequest) {
        lower.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(c))));
    }

    // Must start with GET
    if (lower.rfind("get ", 0) != 0) return false;

    // Must contain Upgrade: websocket
    if (lower.find("upgrade: websocket") == std::string::npos) return false;

    // Must contain Connection header mentioning upgrade
    if (lower.find("connection:") == std::string::npos) return false;

    return true;
}

bool WebSocketProtocol::parseHandshake(const std::string& httpRequest,
                                       WSHandshake& handshake) {
    std::istringstream iss(httpRequest);
    std::string line;

    // Request line  GET /path HTTP/1.1
    if (!std::getline(iss, line)) return false;
    if (!line.empty() && line.back() == '\r') line.pop_back();

    {
        std::istringstream rl(line);
        std::string method;
        rl >> method >> handshake.path;
        // We do not require the method to be GET here; the caller checks
        // isWebSocketUpgrade first.
    }

    // Headers
    handshake.headers.clear();
    while (std::getline(iss, line)) {
        if (!line.empty() && line.back() == '\r') line.pop_back();
        if (line.empty()) break;

        size_t colon = line.find(':');
        if (colon == std::string::npos) continue;

        std::string key = line.substr(0, colon);
        std::string val = line.substr(colon + 1);

        // Trim leading whitespace from value
        size_t start = val.find_first_not_of(" \t");
        if (start != std::string::npos) {
            val = val.substr(start);
        }

        // Normalize header key to Title-Case for consistent lookup
        std::string normKey;
        bool nextUpper = true;
        for (char c : key) {
            if (c == '-') {
                nextUpper = true;
                normKey += c;
            } else if (nextUpper) {
                normKey += static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
                nextUpper = false;
            } else {
                normKey += static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
            }
        }

        handshake.headers[normKey] = val;
    }

    // Extract well-known headers
    auto it = handshake.headers.find("Sec-WebSocket-Key");
    if (it != handshake.headers.end()) {
        handshake.clientKey = it->second;
    }

    it = handshake.headers.find("Sec-WebSocket-Version");
    if (it != handshake.headers.end()) {
        handshake.version = it->second;
    }

    it = handshake.headers.find("Host");
    if (it != handshake.headers.end()) {
        handshake.host = it->second;
    }

    return !handshake.clientKey.empty();
}

std::string WebSocketProtocol::computeAcceptKey(const std::string& clientKey) {
    // RFC 6455 Section 4.2.2
    static const std::string GUID = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";

    std::string combined = clientKey + GUID;

    unsigned char digest[SHA_DIGEST_LENGTH];
    SHA1(reinterpret_cast<const unsigned char*>(combined.data()),
         combined.size(),
         digest);

    return base64Encode(digest, SHA_DIGEST_LENGTH);
}

std::string WebSocketProtocol::createHandshakeResponse(const std::string& clientKey) {
    std::string acceptKey = computeAcceptKey(clientKey);

    std::ostringstream oss;
    oss << "HTTP/1.1 101 Switching Protocols\r\n"
        << "Upgrade: websocket\r\n"
        << "Connection: Upgrade\r\n"
        << "Sec-WebSocket-Accept: " << acceptKey << "\r\n"
        << "\r\n";

    return oss.str();
}

// ============================================================================
// Frame encoding (RFC 6455 Section 5.2)
// ============================================================================

std::vector<uint8_t> WebSocketProtocol::encodeFrame(WSOpcode opcode,
                                                     const std::vector<uint8_t>& payload,
                                                     bool mask) {
    std::vector<uint8_t> frame;

    // Byte 1: FIN + RSV1-3 (0) + opcode
    uint8_t b1 = 0x80 | (static_cast<uint8_t>(opcode) & 0x0F);
    frame.push_back(b1);

    // Byte 2: MASK + payload length
    size_t len = payload.size();
    if (mask) {
        if (len <= 125) {
            frame.push_back(static_cast<uint8_t>(0x80 | len));
        } else if (len <= 65535) {
            frame.push_back(0x80 | 126);
            frame.push_back(static_cast<uint8_t>((len >> 8) & 0xFF));
            frame.push_back(static_cast<uint8_t>(len & 0xFF));
        } else {
            frame.push_back(0x80 | 127);
            for (int i = 7; i >= 0; --i) {
                frame.push_back(static_cast<uint8_t>((len >> (i * 8)) & 0xFF));
            }
        }

        // Generate a random 4-byte masking key
        uint32_t maskKey = static_cast<uint32_t>(std::rand());
        frame.push_back(static_cast<uint8_t>((maskKey >> 24) & 0xFF));
        frame.push_back(static_cast<uint8_t>((maskKey >> 16) & 0xFF));
        frame.push_back(static_cast<uint8_t>((maskKey >> 8) & 0xFF));
        frame.push_back(static_cast<uint8_t>(maskKey & 0xFF));

        // Append masked payload
        std::vector<uint8_t> masked = payload;
        applyMask(masked, maskKey);
        frame.insert(frame.end(), masked.begin(), masked.end());
    } else {
        // Server frames are NOT masked per RFC 6455 Section 5.3
        if (len <= 125) {
            frame.push_back(static_cast<uint8_t>(len));
        } else if (len <= 65535) {
            frame.push_back(126);
            frame.push_back(static_cast<uint8_t>((len >> 8) & 0xFF));
            frame.push_back(static_cast<uint8_t>(len & 0xFF));
        } else {
            frame.push_back(127);
            for (int i = 7; i >= 0; --i) {
                frame.push_back(static_cast<uint8_t>((len >> (i * 8)) & 0xFF));
            }
        }

        frame.insert(frame.end(), payload.begin(), payload.end());
    }

    return frame;
}

std::vector<uint8_t> WebSocketProtocol::encodeTextFrame(const std::string& text,
                                                         bool mask) {
    std::vector<uint8_t> payload(text.begin(), text.end());
    return encodeFrame(WSOpcode::Text, payload, mask);
}

std::vector<uint8_t> WebSocketProtocol::encodeBinaryFrame(const std::vector<uint8_t>& data,
                                                           bool mask) {
    return encodeFrame(WSOpcode::Binary, data, mask);
}

std::vector<uint8_t> WebSocketProtocol::encodeCloseFrame(uint16_t code,
                                                          const std::string& reason) {
    std::vector<uint8_t> payload;
    payload.push_back(static_cast<uint8_t>((code >> 8) & 0xFF));
    payload.push_back(static_cast<uint8_t>(code & 0xFF));
    payload.insert(payload.end(), reason.begin(), reason.end());
    return encodeFrame(WSOpcode::Close, payload, false);
}

std::vector<uint8_t> WebSocketProtocol::encodePingFrame(const std::string& data) {
    std::vector<uint8_t> payload(data.begin(), data.end());
    return encodeFrame(WSOpcode::Ping, payload, false);
}

std::vector<uint8_t> WebSocketProtocol::encodePongFrame(const std::string& data) {
    std::vector<uint8_t> payload(data.begin(), data.end());
    return encodeFrame(WSOpcode::Pong, payload, false);
}

// ============================================================================
// Frame decoding (RFC 6455 Section 5.2)
// ============================================================================

bool WebSocketProtocol::tryDecodeFrame(const std::vector<uint8_t>& data,
                                        WSFrame& frame,
                                        size_t& bytesConsumed) {
    bytesConsumed = 0;

    if (data.size() < 2) return false;

    // Byte 0: FIN + RSV + opcode
    frame.fin = (data[0] & 0x80) != 0;
    uint8_t rawOpcode = data[0] & 0x0F;

    // Validate opcode
    switch (rawOpcode) {
        case 0x0: frame.opcode = WSOpcode::Continuation; break;
        case 0x1: frame.opcode = WSOpcode::Text;         break;
        case 0x2: frame.opcode = WSOpcode::Binary;       break;
        case 0x8: frame.opcode = WSOpcode::Close;        break;
        case 0x9: frame.opcode = WSOpcode::Ping;         break;
        case 0xA: frame.opcode = WSOpcode::Pong;         break;
        default:
            // Reserved / invalid opcode
            return false;
    }

    // Byte 1: MASK + initial length
    frame.masked = (data[1] & 0x80) != 0;
    uint64_t payloadLen = data[1] & 0x7F;

    size_t headerSize = 2; // minimum

    if (payloadLen == 126) {
        // 16-bit length follows
        if (data.size() < 4) return false;
        payloadLen = (static_cast<uint64_t>(data[2]) << 8) | data[3];
        headerSize = 4;
    } else if (payloadLen == 127) {
        // 64-bit length follows
        if (data.size() < 10) return false;
        payloadLen = 0;
        for (int i = 0; i < 8; ++i) {
            payloadLen = (payloadLen << 8) | data[2 + i];
        }
        headerSize = 10;
    }

    // Masking key
    size_t maskOffset = headerSize;
    if (frame.masked) {
        headerSize += 4;
        if (data.size() < headerSize) return false;
    }

    // Total frame size
    if (data.size() < headerSize + payloadLen) return false;

    // Extract payload
    frame.payload.assign(data.begin() + headerSize,
                         data.begin() + headerSize + payloadLen);

    // Unmask if needed
    if (frame.masked) {
        std::vector<uint8_t> maskKey(data.begin() + maskOffset,
                                     data.begin() + maskOffset + 4);
        applyMask(frame.payload, maskKey);
    }

    bytesConsumed = headerSize + payloadLen;
    return true;
}

std::vector<WSFrame> WebSocketProtocol::decodeFrames(const std::vector<uint8_t>& data) {
    std::vector<WSFrame> frames;
    size_t offset = 0;

    while (offset < data.size()) {
        std::vector<uint8_t> remaining(data.begin() + offset, data.end());
        WSFrame frame;
        size_t consumed = 0;

        if (!tryDecodeFrame(remaining, frame, consumed)) {
            break; // Incomplete frame -- wait for more data
        }

        frames.push_back(std::move(frame));
        offset += consumed;
    }

    return frames;
}

// ============================================================================
// Masking (RFC 6455 Section 5.3)
// ============================================================================

void WebSocketProtocol::applyMask(std::vector<uint8_t>& payload, uint32_t maskKey) {
    uint8_t key[4];
    key[0] = static_cast<uint8_t>((maskKey >> 24) & 0xFF);
    key[1] = static_cast<uint8_t>((maskKey >> 16) & 0xFF);
    key[2] = static_cast<uint8_t>((maskKey >> 8) & 0xFF);
    key[3] = static_cast<uint8_t>(maskKey & 0xFF);

    for (size_t i = 0; i < payload.size(); ++i) {
        payload[i] ^= key[i % 4];
    }
}

void WebSocketProtocol::applyMask(std::vector<uint8_t>& payload,
                                   const std::vector<uint8_t>& maskingKey) {
    if (maskingKey.size() < 4) return;

    for (size_t i = 0; i < payload.size(); ++i) {
        payload[i] ^= maskingKey[i % 4];
    }
}

// ============================================================================
// Close codes (RFC 6455 Section 7.4.1)
// ============================================================================

std::string WebSocketProtocol::closeCodeToString(uint16_t code) {
    switch (code) {
        case 1000: return "Normal closure";
        case 1001: return "Going away";
        case 1002: return "Protocol error";
        case 1003: return "Unsupported data";
        case 1005: return "No status received";
        case 1006: return "Abnormal closure";
        case 1007: return "Invalid frame payload data";
        case 1008: return "Policy violation";
        case 1009: return "Message too big";
        case 1010: return "Mandatory extension";
        case 1011: return "Internal server error";
        case 1012: return "Service restart";
        case 1013: return "Try again later";
        case 1015: return "TLS handshake failure";
        default:   return "Unknown close code";
    }
}

} // namespace PaperCrawler
