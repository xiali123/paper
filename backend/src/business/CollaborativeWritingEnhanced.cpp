#include "business/CollaborativeWritingEnhanced.hpp"
#include "modules/LoggingModule.hpp"
#include "business/UnifiedAIWorkflow.hpp"
#include "data/IDatabase.hpp"
#include "data/PreparedStatement.hpp"
using DataPreparedStatement = PreparedStatement;
#include "data/QueryBuilder.hpp"
#include "network/WebSocketModule.hpp"
#include <sstream>
#include <algorithm>
#include <chrono>
#include <iomanip>
#include <stdexcept>

namespace PaperCrawler {

// ============================================================================
// CollaborationSessionManager Implementation
// ============================================================================

CollaborationSessionManager::CollaborationSessionManager(
    std::shared_ptr<WebSocketModule> websocketModule)
    : websocketModule_(websocketModule) {

    // 启动心跳检测定时器（每30秒）
    // 后续集成到SchedulerModule统一调度
}

void CollaborationSessionManager::addConnection(
    int documentId,
    int userId,
    const std::string& socketId) {

    try {
        auto logging = Services::resolve<LoggingModule>();
        if (logging) {
            logging->info("Adding WebSocket connection - DocumentID: " + std::to_string(documentId) +
                         ", UserID: " + std::to_string(userId) +
                         ", SocketID: " + socketId);
        }

        std::lock_guard<std::mutex> lock(mutex_);

        // 检查是否已存在
        if (connections_.find(socketId) != connections_.end()) {
            if (logging) {
                logging->warn("Connection already exists: " + socketId);
            }
            return;
        }

        // 创建新连接
        WebSocketConnection conn;
        conn.socketId = socketId;
        conn.userId = userId;
        conn.documentId = documentId;
        conn.isActive = true;
        conn.lastHeartbeat = getCurrentTimestamp();

        connections_[socketId] = conn;
        documentConnections_[documentId].push_back(socketId);

        if (logging) {
            logging->info("Connection added successfully. Total connections for document " +
                         std::to_string(documentId) + ": " +
                         std::to_string(documentConnections_[documentId].size()));
        }

        // 广播用户加入通知
        std::ostringstream message;
        message << "{";
        message << "\"type\":\"user_joined\",";
        message << "\"userId\":" << userId << ",";
        message << "\"socketId\":\"" << socketId << "\",";
        message << "\"timestamp\":\"" << getCurrentTimestamp() << "\"";
        message << "}";

        broadcastOperation(documentId, message.str());

    } catch (const std::exception& e) {
        auto logging = Services::resolve<LoggingModule>();
        if (logging) {
            logging->error("Failed to add connection: " + std::string(e.what()));
        }
        throw std::runtime_error("Failed to add WebSocket connection: " + std::string(e.what()));
    }
}

void CollaborationSessionManager::removeConnection(const std::string& socketId) {
    std::lock_guard<std::mutex> lock(mutex_);

    auto it = connections_.find(socketId);
    if (it != connections_.end()) {
        int documentId = it->second.documentId;
        int userId = it->second.userId;

        // 从文档连接列表中移除
        auto& docConns = documentConnections_[documentId];
        docConns.erase(
            std::remove(docConns.begin(), docConns.end(), socketId),
            docConns.end()
        );

        // 移除光标位置
        cursorPositions_[documentId].erase(userId);

        // 广播用户离开通知
        std::ostringstream message;
        message << "{";
        message << "\"type\":\"user_left\",";
        message << "\"userId\":" << userId << ",";
        message << "\"socketId\":\"" << socketId << "\",";
        message << "\"timestamp\":\"" << getCurrentTimestamp() << "\"";
        message << "}";

        broadcastOperation(documentId, message.str());

        // 移除连接
        connections_.erase(it);
    }
}

void CollaborationSessionManager::broadcastOperation(
    int documentId,
    const std::string& message) {

    try {
        if (!websocketModule_) {
            auto logging = Services::resolve<LoggingModule>();
            if (logging) {
                logging->warn("WebSocketModule not available for broadcasting");
            }
            return;
        }

        std::lock_guard<std::mutex> lock(mutex_);

        auto it = documentConnections_.find(documentId);
        if (it == documentConnections_.end()) {
            return;
        }

        size_t successCount = 0;
        for (const auto& socketId : it->second) {
            if (websocketModule_->send(socketId, message)) {
                successCount++;
            } else {
                auto logging = Services::resolve<LoggingModule>();
                if (logging) {
                    logging->warn("Failed to send message to socket: " + socketId);
                }
            }
        }

        auto logging = Services::resolve<LoggingModule>();
        if (logging) {
            logging->info("Broadcasted message to " + std::to_string(successCount) +
                         "/" + std::to_string(it->second.size()) +
                         " connections for document " + std::to_string(documentId));
        }

    } catch (const std::exception& e) {
        auto logging = Services::resolve<LoggingModule>();
        if (logging) {
            logging->error("Failed to broadcast operation: " + std::string(e.what()));
        }
    }
}

void CollaborationSessionManager::sendToUser(
    int userId,
    const std::string& message) {

    try {
        if (!websocketModule_) {
            return;
        }

        std::lock_guard<std::mutex> lock(mutex_);

        size_t successCount = 0;
        for (const auto& [socketId, conn] : connections_) {
            if (conn.userId == userId && conn.isActive) {
                if (websocketModule_->send(socketId, message)) {
                    successCount++;
                }
            }
        }

        auto logging = Services::resolve<LoggingModule>();
        if (logging && successCount > 0) {
            logging->info("Sent message to user " + std::to_string(userId) +
                         " via " + std::to_string(successCount) + " connections");
        }

    } catch (const std::exception& e) {
        auto logging = Services::resolve<LoggingModule>();
        if (logging) {
            logging->error("Failed to send message to user: " + std::string(e.what()));
        }
    }
}

void CollaborationSessionManager::updateCursor(
    int documentId,
    int userId,
    int position) {

    std::lock_guard<std::mutex> lock(mutex_);

    cursorPositions_[documentId][userId] = position;

    // 广播光标位置更新
    std::ostringstream message;
    message << "{";
    message << "\"type\":\"cursor_update\",";
    message << "\"userId\":" << userId << ",";
    message << "\"position\":" << position << ",";
    message << "\"timestamp\":\"" << getCurrentTimestamp() << "\"";
    message << "}";

    broadcastOperation(documentId, message.str());
}

void CollaborationSessionManager::broadcastCursorUpdate(
    int documentId,
    int userId,
    int position) {

    updateCursor(documentId, userId, position);
}

void CollaborationSessionManager::heartbeatCheck() {
    try {
        std::lock_guard<std::mutex> lock(mutex_);

        auto now = std::chrono::system_clock::now();
        std::vector<std::string> staleSockets;
        const std::chrono::seconds TIMEOUT_THRESHOLD(60); // 60秒超时

        for (const auto& [socketId, conn] : connections_) {
            if (!conn.isActive) {
                continue;
            }

            // 解析时间戳
            std::tm tm = {};
            std::istringstream iss(conn.lastHeartbeat);
            iss >> std::get_time(&tm, "%Y-%m-%d %H:%M:%S");

            if (iss.fail()) {
                auto logging = Services::resolve<LoggingModule>();
                if (logging) {
                    logging->warn("Failed to parse heartbeat timestamp for socket: " + socketId);
                }
                continue;
            }

            auto lastHeartbeatTime = std::chrono::system_clock::from_time_t(std::mktime(&tm));
            auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - lastHeartbeatTime);

            if (elapsed > TIMEOUT_THRESHOLD) {
                staleSockets.push_back(socketId);

                auto logging = Services::resolve<LoggingModule>();
                if (logging) {
                    logging->warn("Connection timeout detected: " + socketId +
                                 " (inactive for " + std::to_string(elapsed.count()) + " seconds)");
                }
            }
        }

        // 移除超时连接
        for (const auto& socketId : staleSockets) {
            removeConnection(socketId);
        }

        if (!staleSockets.empty()) {
            auto logging = Services::resolve<LoggingModule>();
            if (logging) {
                logging->info("Heartbeat check completed. Removed " +
                             std::to_string(staleSockets.size()) + " stale connections. " +
                             "Active connections: " + std::to_string(connections_.size()));
            }
        }

    } catch (const std::exception& e) {
        auto logging = Services::resolve<LoggingModule>();
        if (logging) {
            logging->error("Heartbeat check failed: " + std::string(e.what()));
        }
    }
}

int CollaborationSessionManager::getActiveConnections(int documentId) {
    std::lock_guard<std::mutex> lock(mutex_);

    auto it = documentConnections_.find(documentId);
    if (it != documentConnections_.end()) {
        int count = 0;
        for (const auto& socketId : it->second) {
            auto connIt = connections_.find(socketId);
            if (connIt != connections_.end() && connIt->second.isActive) {
                count++;
            }
        }
        return count;
    }

    return 0;
}

std::string CollaborationSessionManager::getCurrentTimestamp() {
    auto now = std::chrono::system_clock::now();
    auto time = std::chrono::system_clock::to_time_t(now);

    std::ostringstream oss;
    oss << std::put_time(std::localtime(&time), "%Y-%m-%d %H:%M:%S");
    return oss.str();
}

// ============================================================================
// RealTimeCollaborationEditor Implementation
// ============================================================================

RealTimeCollaborationEditor::RealTimeCollaborationEditor(
    std::shared_ptr<IDatabase> database)
    : database_(database) {

    // 从数据库加载最近的文档内容到缓存
    loadDocumentsFromDatabase();
}

std::string RealTimeCollaborationEditor::applyOperation(
    int documentId,
    const OTOperation& operation) {

    try {
        auto logging = Services::resolve<LoggingModule>();

        std::lock_guard<std::mutex> lock(mutex_);

        // 获取当前文档内容
        std::string content = getDocumentContent(documentId);

        if (content.empty()) {
            if (logging) {
                logging->warn("Document content is empty for document ID: " + std::to_string(documentId));
            }
        }

        // 应用操作
        std::string newContent;
        bool operationValid = true;

        switch (operation.type) {
            case OTOperationType::INSERT:
                if (operation.position >= 0 && operation.position <= static_cast<int>(content.length())) {
                    newContent = content.substr(0, operation.position) +
                                  operation.content +
                                  content.substr(operation.position);

                    if (logging) {
                        logging->debug("INSERT operation applied at position " +
                                     std::to_string(operation.position) +
                                     ", content length: " + std::to_string(operation.content.length()));
                    }
                } else {
                    operationValid = false;
                    if (logging) {
                        logging->error("Invalid INSERT position: " + std::to_string(operation.position) +
                                     ", content length: " + std::to_string(content.length()));
                    }
                }
                break;

            case OTOperationType::DELETE:
                if (operation.position >= 0 &&
                    operation.position + operation.length <= static_cast<int>(content.length())) {
                    newContent = content.substr(0, operation.position) +
                                  content.substr(operation.position + operation.length);

                    if (logging) {
                        logging->debug("DELETE operation applied at position " +
                                     std::to_string(operation.position) +
                                     ", length: " + std::to_string(operation.length));
                    }
                } else {
                    operationValid = false;
                    if (logging) {
                        logging->error("Invalid DELETE operation: position=" +
                                     std::to_string(operation.position) +
                                     ", length=" + std::to_string(operation.length) +
                                     ", content length=" + std::to_string(content.length()));
                    }
                }
                break;

            case OTOperationType::RETAIN:
                newContent = content;
                break;

            default:
                operationValid = false;
                if (logging) {
                    logging->warn("Unknown operation type: " + std::to_string(static_cast<int>(operation.type)));
                }
                break;
        }

        if (!operationValid) {
            return content; // 返回原内容
        }

        // 更新缓存
        documentContents_[documentId] = newContent;

        // 记录操作到历史（保留最近1000个操作）
        operationHistory_[documentId].push_back(operation);
        if (operationHistory_[documentId].size() > 1000) {
            operationHistory_[documentId].erase(operationHistory_[documentId].begin());
        }

        // 持久化到数据库
        PreparedStatement stmt(database_,
            "UPDATE collaborative_documents SET content = ?, word_count = ?, updated_at = NOW() WHERE id = ?"
        );

        stmt.bind(1, newContent);
        stmt.bind(2, static_cast<int>(std::count_if(newContent.begin(), newContent.end(), ::isprint)));
        stmt.bind(3, documentId);

        if (!stmt.execute()) {
            if (logging) {
                logging->error("Failed to persist document operation to database");
            }
        } else {
            if (logging) {
                logging->info("Document operation applied successfully. Document ID: " +
                             std::to_string(documentId) +
                             ", new content length: " + std::to_string(newContent.length()));
            }
        }

        return newContent;

    } catch (const std::exception& e) {
        auto logging = Services::resolve<LoggingModule>();
        if (logging) {
            logging->error("Failed to apply operation: " + std::string(e.what()));
        }
        throw std::runtime_error("Failed to apply operation: " + std::string(e.what()));
    }
}

OTOperation RealTimeCollaborationEditor::transformClientOperation(
    const OTOperation& clientOp,
    int documentId) {

    std::lock_guard<std::mutex> lock(mutex_);

    // 获取该文档的操作历史
    auto& history = operationHistory_[documentId];

    // 对操作历史中的每个操作进行转换
    OTOperation transformedOp = clientOp;
    for (const auto& serverOp : history) {
        transformedOp = transformOperationHelper(transformedOp, serverOp);
    }

    return transformedOp;
}

std::string RealTimeCollaborationEditor::getDocumentContent(int documentId) {
    std::lock_guard<std::mutex> lock(mutex_);

    auto it = documentContents_.find(documentId);
    if (it != documentContents_.end()) {
        return it->second;
    }

    // 从数据库加载
    QueryBuilder queryBuilder(database_);
    queryBuilder.select("content")
        .from("collaborative_documents")
        .where("id", "=", documentId);

    auto rows = queryBuilder.query();
    if (!rows.empty() && !rows[0].empty()) {
        documentContents_[documentId] = rows[0].begin()->second;
        return documentContents_[documentId];
    }

    return "";
}

bool RealTimeCollaborationEditor::saveSnapshot(
    int documentId,
    const std::string& content,
    int userId) {

    try {
        // 创建版本快照
        PreparedStatement stmt(database_,
            "INSERT INTO document_versions "
            "(document_id, version_number, content, created_by, change_summary) "
            "VALUES (?, (SELECT COALESCE(MAX(version_number), 0) + 1 FROM document_versions WHERE document_id = ?), ?, ?, ?)"
        );

        stmt.bind(1, documentId);
        stmt.bind(2, documentId);
        stmt.bind(3, content);
        stmt.bind(4, userId);
        stmt.bind(5, "Manual snapshot");

        if (stmt.execute()) {
            // 更新缓存
            std::lock_guard<std::mutex> lock(mutex_);
            documentContents_[documentId] = content;
            return true;
        }

    } catch (const std::exception& e) {
        if (auto logging = Services::resolve<LoggingModule>()) {
            logging->error("Failed to save snapshot: " + std::string(e.what()));
        }
    }

    return false;
}

OTOperation RealTimeCollaborationEditor::transformOperationHelper(
    const OTOperation& clientOp,
    const OTOperation& serverOp) {

    // 基于操作类型调用相应的转换函数
    if (clientOp.type == OTOperationType::INSERT) {
        if (serverOp.type == OTOperationType::INSERT) {
            return transformInsertAgainstInsert(clientOp, serverOp);
        } else if (serverOp.type == OTOperationType::DELETE) {
            return transformInsertAgainstDelete(clientOp, serverOp);
        }
    } else if (clientOp.type == OTOperationType::DELETE) {
        if (serverOp.type == OTOperationType::INSERT) {
            return transformDeleteAgainstInsert(clientOp, serverOp);
        } else if (serverOp.type == OTOperationType::DELETE) {
            return transformDeleteAgainstDelete(clientOp, serverOp);
        }
    }

    return clientOp;
}

OTOperation RealTimeCollaborationEditor::transformInsertAgainstInsert(
    const OTOperation& clientOp,
    const OTOperation& serverOp) {

    OTOperation transformed = clientOp;

    if (serverOp.position <= clientOp.position) {
        // 服务器操作在客户端操作之前
        transformed.position = clientOp.position + static_cast<int>(serverOp.content.length());
    }

    return transformed;
}

OTOperation RealTimeCollaborationEditor::transformInsertAgainstDelete(
    const OTOperation& clientOp,
    const OTOperation& serverOp) {

    OTOperation transformed = clientOp;

    if (serverOp.position < clientOp.position) {
        // 服务器删除在客户端插入之前
        int offset = std::min(serverOp.length, clientOp.position - serverOp.position);
        transformed.position = clientOp.position - offset;
    } else if (serverOp.position < clientOp.position + static_cast<int>(clientOp.content.length())) {
        // 服务器删除与客户端插入重叠
        int overlapStart = std::max(serverOp.position, clientOp.position);
        int overlapEnd = std::min(serverOp.position + serverOp.length,
                                clientOp.position + static_cast<int>(clientOp.content.length()));
        int overlapLength = overlapEnd - overlapStart;

        if (overlapLength > 0) {
            transformed.content = clientOp.content.substr(0, overlapStart - clientOp.position) +
                                  clientOp.content.substr(overlapEnd - clientOp.position);
        }
    }

    return transformed;
}

OTOperation RealTimeCollaborationEditor::transformDeleteAgainstInsert(
    const OTOperation& clientOp,
    const OTOperation& serverOp) {

    OTOperation transformed = clientOp;

    if (serverOp.position <= clientOp.position) {
        // 服务器插入在客户端删除之前
        transformed.position = clientOp.position + static_cast<int>(serverOp.content.length());
    }

    return transformed;
}

OTOperation RealTimeCollaborationEditor::transformDeleteAgainstDelete(
    const OTOperation& clientOp,
    const OTOperation& serverOp) {

    OTOperation transformed = clientOp;

    if (serverOp.position < clientOp.position + clientOp.length &&
        serverOp.position + serverOp.length > clientOp.position) {
        // 重叠：缩短客户端删除的长度
        int overlapStart = std::max(serverOp.position, clientOp.position);
        int overlapEnd = std::min(serverOp.position + serverOp.length,
                                clientOp.position + clientOp.length);
        int removedByServer = overlapEnd - overlapStart;

        transformed.length = clientOp.length - removedByServer;
        if (transformed.length <= 0) {
            transformed.type = OTOperationType::RETAIN;
            transformed.length = 0;
        }
    }

    return transformed;
}

void RealTimeCollaborationEditor::loadDocumentsFromDatabase() {
    // 从数据库加载最近的文档内容到缓存（取消注释即可启用）
    // QueryBuilder queryBuilder(database_);
    // queryBuilder.select().from("collaborative_documents").limit(100);
    // auto rows = queryBuilder.query();
    // for (const auto& row : rows) {
    //     int id = std::stoi(row.at("id"));
    //     documentContents_[id] = row.at("content");
    // }
}

// ============================================================================
// AIWritingAssistant Implementation
// ============================================================================

AIWritingAssistant::AIWritingAssistant(std::shared_ptr<UnifiedAIWorkflow> aiWorkflow)
    : aiWorkflow_(aiWorkflow) {

}

WritingSuggestion AIWritingAssistant::generateSuggestion(
    int documentId,
    const std::string& content,
    int position,
    const std::string& suggestionType) {

    WritingSuggestion suggestion;
    suggestion.documentId = documentId;
    suggestion.suggestionType = suggestionType;
    suggestion.positionStart = position;
    suggestion.positionEnd = position + 100; // 默认100字符上下文
    suggestion.confidenceScore = 0.0f;
    suggestion.status = "pending";

    if (!aiWorkflow_) {
        return suggestion;
    }

    try {
        // 提取上下文
        int contextStart = std::max(0, position - 100);
        int contextEnd = std::min(static_cast<int>(content.length()), position + 100);
        std::string context = content.substr(contextStart, contextEnd - contextStart);

        // 构建AI提示词
        std::string prompt = buildSuggestionPrompt(context, position - contextStart, suggestionType);

        // 调用AI
        auto aiResult = aiWorkflow_->executeAIRequest(
            prompt,
            AIModelType::GPT_4_MINI
        );

        if (aiResult.success) {
            suggestion.originalText = content.substr(position, 100);
            suggestion.suggestedText = aiResult.content;
            suggestion.confidenceScore = 0.85f; // 占位：应从AI响应中解析实际置信度
            suggestion.explanation = generateExplanation(suggestionType);
            suggestion.status = "ready";
        }

    } catch (const std::exception& e) {
        if (auto logging = Services::resolve<LoggingModule>()) {
            logging->error("Failed to generate suggestion: " + std::string(e.what()));
        }
    }

    return suggestion;
}

std::vector<std::map<std::string, std::string>> AIWritingAssistant::checkGrammar(
    const std::string& text) {

    std::vector<std::map<std::string, std::string>> errors;

    if (!aiWorkflow_) {
        return errors;
    }

    try {
        // 构建语法检查提示词
        std::ostringstream prompt;
        prompt << "Please check the following text for grammar errors, spelling mistakes, ";
        prompt << "and stylistic issues. Return results in JSON format with these fields:\n";
        prompt << "[{\"error\": \"error text\", \"correction\": \"corrected text\", ";
        prompt << "\"position\": start_index, \"type\": \"grammar|spelling|style\", ";
        prompt << "\"explanation\": \"brief explanation\"}]\n\n";
        prompt << "Text to check:\n" << text;

        // 调用AI
        auto aiResult = aiWorkflow_->executeAIRequest(
            prompt.str(),
            AIModelType::GPT_4_MINI
        );

        if (aiResult.success) {
            // 解析AI返回的JSON结果（取消注释即可启用）
            // auto jsonErrors = JsonUtils::parse(aiResult.content);
            // for (const auto& error : jsonErrors) {
            //     std::map<std::string, std::string> errorMap;
            //     errorMap["error"] = JsonUtils::getValue<std::string>(error, "error").value();
            //     errorMap["correction"] = JsonUtils::getValue<std::string>(error, "correction").value();
            //     errorMap["position"] = std::to_string(JsonUtils::getValue<int>(error, "position").value());
            //     errorMap["type"] = JsonUtils::getValue<std::string>(error, "type").value();
            //     errorMap["explanation"] = JsonUtils::getValue<std::string>(error, "explanation").value();
            //     errors.push_back(errorMap);
            // }
        }

    } catch (const std::exception& e) {
        if (auto logging = Services::resolve<LoggingModule>()) {
            logging->error("Failed to check grammar: " + std::string(e.what()));
        }
    }

    return errors;
}

std::vector<std::string> AIWritingAssistant::improveStyle(
    const std::string& text,
    const std::string& style) {

    std::vector<std::string> suggestions;

    if (!aiWorkflow_) {
        return suggestions;
    }

    try {
        // 构建风格改进提示词
        std::ostringstream prompt;
        prompt << "Please improve the following text to match the \"" << style << "\" style. ";
        prompt << "Provide 3-5 alternative versions with different approaches.\n\n";
        prompt << "Original text:\n" << text << "\n\n";
        prompt << "Return results in JSON format: ";
        prompt << "[{\"version\": \"improved text 1\", \"approach\": \"brief description\"}, ...]";

        // 调用AI
        auto aiResult = aiWorkflow_->executeAIRequest(
            prompt.str(),
            AIModelType::GPT_4_MINI
        );

        if (aiResult.success) {
            // 解析AI返回的JSON结果（取消注释即可启用）
            // auto jsonSuggestions = JsonUtils::parse(aiResult.content);
            // for (const auto& sug : jsonSuggestions) {
            //     suggestions.push_back(JsonUtils::getValue<std::string>(sug, "version").value());
            // }
        }

    } catch (const std::exception& e) {
        if (auto logging = Services::resolve<LoggingModule>()) {
            logging->error("Failed to improve style: " + std::string(e.what()));
        }
    }

    return suggestions;
}

std::vector<std::map<std::string, std::string>> AIWritingAssistant::recommendCitations(
    const std::string& context,
    int maxCount) {

    std::vector<std::map<std::string, std::string>> citations;

    if (!aiWorkflow_) {
        return citations;
    }

    try {
        // 构建引用推荐提示词
        std::ostringstream prompt;
        prompt << "Based on the following context, recommend " << maxCount << " relevant academic papers. ";
        prompt << "For each paper, provide title, authors, year, and relevance score (0-1).\n\n";
        prompt << "Context:\n" << context << "\n\n";
        prompt << "Return results in JSON format: ";
        prompt << "[{\"title\": \"paper title\", \"authors\": \"author names\", ";
        prompt << "\"year\": 2024, \"relevance\": 0.95, \"reason\": \"brief explanation\"}, ...]";

        // 调用AI
        auto aiResult = aiWorkflow_->executeAIRequest(
            prompt.str(),
            AIModelType::GPT_4
        );

        if (aiResult.success) {
            // 解析AI返回的JSON结果并查询知识图谱（取消注释即可启用）
            // auto jsonCitations = JsonUtils::parse(aiResult.content);
            // for (const auto& cit : jsonCitations) {
            //     std::map<std::string, std::string> citation;
            //     citation["title"] = JsonUtils::getValue<std::string>(cit, "title").value();
            //     citation["authors"] = JsonUtils::getValue<std::string>(cit, "authors").value();
            //     citation["year"] = std::to_string(JsonUtils::getValue<int>(cit, "year").value());
            //     citation["relevance"] = std::to_string(JsonUtils::getValue<double>(cit, "relevance").value());
            //     citation["reason"] = JsonUtils::getValue<std::string>(cit, "reason").value();
            //     citations.push_back(citation);
            // }
        }

    } catch (const std::exception& e) {
        if (auto logging = Services::resolve<LoggingModule>()) {
            logging->error("Failed to recommend citations: " + std::string(e.what()));
        }
    }

    return citations;
}

std::vector<std::string> AIWritingAssistant::suggestAutocompletion(
    const std::string& prefix,
    const std::string& documentContext) {

    std::vector<std::string> completions;

    if (!aiWorkflow_) {
        return completions;
    }

    try {
        // 构建自动完成提示词
        std::ostringstream prompt;
        prompt << "Complete the following text based on the document context. ";
        prompt << "Provide 5-10 likely completions for academic writing.\n\n";
        prompt << "Document context:\n" << documentContext << "\n\n";
        prompt << "Prefix to complete:\n" << prefix << "\n\n";
        prompt << "Return results in JSON array: [\"completion 1\", \"completion 2\", ...]";

        // 调用AI
        auto aiResult = aiWorkflow_->executeAIRequest(
            prompt.str(),
            AIModelType::GPT_4_MINI
        );

        if (aiResult.success) {
            // 解析AI返回的JSON结果（取消注释即可启用）
            // auto jsonCompletions = JsonUtils::parse(aiResult.content);
            // for (const auto& comp : jsonCompletions) {
            //     completions.push_back(comp.get<std::string>());
            // }
        }

    } catch (const std::exception& e) {
        if (auto logging = Services::resolve<LoggingModule>()) {
            logging->error("Failed to suggest autocompletion: " + std::string(e.what()));
        }
    }

    return completions;
}

std::string AIWritingAssistant::buildSuggestionPrompt(
    const std::string& context,
    int position,
    const std::string& suggestionType) {

    std::ostringstream prompt;

    if (suggestionType == "grammar") {
        prompt << "Please suggest grammar improvements for the text around position " << position << ".\n";
        prompt << "Context:\n" << context;
    } else if (suggestionType == "style") {
        prompt << "Please suggest style improvements for academic writing at position " << position << ".\n";
        prompt << "Context:\n" << context;
    } else if (suggestionType == "clarity") {
        prompt << "Please suggest clarity improvements at position " << position << ".\n";
        prompt << "Context:\n" << context;
    } else {
        prompt << "Please provide writing suggestions for the following text:\n" << context;
    }

    return prompt.str();
}

std::string AIWritingAssistant::generateExplanation(const std::string& suggestionType) {
    if (suggestionType == "grammar") {
        return "Grammar improvement suggestion based on academic writing standards";
    } else if (suggestionType == "style") {
        return "Style improvement suggestion for better academic flow";
    } else if (suggestionType == "clarity") {
        return "Clarity improvement to enhance reader understanding";
    } else {
        return "AI-powered writing suggestion";
    }
}

} // namespace PaperCrawler
