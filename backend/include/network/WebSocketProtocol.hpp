#pragma once

#include <vector>
#include <string>
#include <cstdint>
#include <map>

namespace PaperCrawler {

// WebSocket opcode per RFC 6455 Section 5.2
enum class WSOpcode : uint8_t {
    Continuation = 0x0,
    Text = 0x1,
    Binary = 0x2,
    Close = 0x8,
    Ping = 0x9,
    Pong = 0xA
};

// WebSocket frame representation
struct WSFrame {
    WSOpcode opcode;
    bool fin;
    bool masked;
    std::vector<uint8_t> payload;
};

// WebSocket handshake data parsed from HTTP upgrade request
struct WSHandshake {
    std::string clientKey;   // Sec-WebSocket-Key
    std::string version;     // Sec-WebSocket-Version
    std::string host;
    std::string path;
    std::map<std::string, std::string> headers;
};

/**
 * RFC 6455 WebSocket protocol implementation.
 *
 * Provides:
 *  - Handshake parsing and response generation (Section 4)
 *  - Frame encoding / decoding (Section 5)
 *  - Masking / unmasking (Section 5.3)
 *  - Ping/Pong/Close control frames
 */
class WebSocketProtocol {
public:
    // ------------------------------------------------------------------
    // Handshake (RFC 6455 Section 4)
    // ------------------------------------------------------------------

    /** Parse an HTTP upgrade request into a WSHandshake struct. */
    static bool parseHandshake(const std::string& httpRequest, WSHandshake& handshake);

    /** Build the HTTP 101 response with Sec-WebSocket-Accept. */
    static std::string createHandshakeResponse(const std::string& clientKey);

    /** Compute the Sec-WebSocket-Accept value from the client key. */
    static std::string computeAcceptKey(const std::string& clientKey);

    /** Return true if the raw HTTP text contains a WebSocket upgrade request. */
    static bool isWebSocketUpgrade(const std::string& httpRequest);

    // ------------------------------------------------------------------
    // Frame encoding (server -> client, never masked)
    // ------------------------------------------------------------------

    static std::vector<uint8_t> encodeFrame(WSOpcode opcode,
                                            const std::vector<uint8_t>& payload,
                                            bool mask = false);

    static std::vector<uint8_t> encodeTextFrame(const std::string& text,
                                                bool mask = false);

    static std::vector<uint8_t> encodeBinaryFrame(const std::vector<uint8_t>& data,
                                                  bool mask = false);

    static std::vector<uint8_t> encodeCloseFrame(uint16_t code = 1000,
                                                 const std::string& reason = "");

    static std::vector<uint8_t> encodePingFrame(const std::string& data = "");

    static std::vector<uint8_t> encodePongFrame(const std::string& data = "");

    // ------------------------------------------------------------------
    // Frame decoding (client -> server, may be masked)
    // ------------------------------------------------------------------

    /** Decode all complete frames in data. Partial frames are ignored. */
    static std::vector<WSFrame> decodeFrames(const std::vector<uint8_t>& data);

    /** Try to decode one frame. Returns false if data is incomplete. */
    static bool tryDecodeFrame(const std::vector<uint8_t>& data,
                               WSFrame& frame,
                               size_t& bytesConsumed);

    // ------------------------------------------------------------------
    // Masking (RFC 6455 Section 5.3)
    // ------------------------------------------------------------------

    static void applyMask(std::vector<uint8_t>& payload, uint32_t maskKey);
    static void applyMask(std::vector<uint8_t>& payload,
                          const std::vector<uint8_t>& maskingKey);

    // ------------------------------------------------------------------
    // Utilities
    // ------------------------------------------------------------------

    static std::string closeCodeToString(uint16_t code);

private:
    // Internal Base64 encoder (uses OpenSSL for SHA-1 externally)
    static std::string base64Encode(const unsigned char* data, size_t len);
};

} // namespace PaperCrawler
