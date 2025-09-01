#pragma once

#include <memory>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <functional>
#include <mutex>
#include <sqlite3.h>
#include <spdlog/spdlog.h>
#include "Database.hpp"
#include "npchat_stub/npchat.hpp"

class MessageService {
private:
  std::shared_ptr<Database> db_;
  mutable std::mutex mutex_;

  // Prepared statements
  sqlite3_stmt* get_undelivered_messages_stmt_;
  sqlite3_stmt* mark_message_read_stmt_;
  sqlite3_stmt* get_unread_count_stmt_;
  sqlite3_stmt* get_last_message_stmt_;
  sqlite3_stmt* delete_message_stmt_;
  sqlite3_stmt* update_message_stmt_;
  sqlite3_stmt* get_message_history_stmt_;
  sqlite3_stmt* search_messages_stmt_;
  sqlite3_stmt* get_chat_last_activity_stmt_;

  // Message delivery callbacks
  std::unordered_map<std::uint32_t, std::function<void(const npchat::ChatMessage&)>> delivery_callbacks_;
  std::unordered_set<std::uint32_t> online_users_;

public:
  explicit MessageService(const std::shared_ptr<Database>& database) : db_(database) {
    get_undelivered_messages_stmt_ = db_->prepareStatement(
      "SELECT m.id, m.chat_id, m.sender_id, m.content, m.timestamp, m.attachment_id, "
      "       u.username, a.type, a.name, a.data "
      "FROM messages m "
      "JOIN users u ON m.sender_id = u.id "
      "LEFT JOIN attachments a ON m.attachment_id = a.id "
      "LEFT JOIN message_delivery md ON m.id = md.message_id AND md.user_id = ? "
      "JOIN chat_participants cp ON m.chat_id = cp.chat_id "
      "WHERE cp.user_id = ? AND md.message_id IS NULL "
      "ORDER BY m.timestamp ASC");

    mark_message_read_stmt_ = db_->prepareStatement(
      "INSERT OR REPLACE INTO message_read (message_id, user_id, read_at) VALUES (?, ?, ?)");

    get_unread_count_stmt_ = db_->prepareStatement(
      "SELECT COUNT(*) FROM messages m "
      "JOIN chat_participants cp ON m.chat_id = cp.chat_id "
      "LEFT JOIN message_read mr ON m.id = mr.message_id AND mr.user_id = ? "
      "WHERE cp.user_id = ? AND mr.message_id IS NULL AND m.sender_id != ?");

    get_last_message_stmt_ = db_->prepareStatement(
      "SELECT m.id, m.chat_id, m.sender_id, m.content, m.timestamp, m.attachment_id, "
      "       u.username, a.type, a.name, a.data "
      "FROM messages m "
      "JOIN users u ON m.sender_id = u.id "
      "LEFT JOIN attachments a ON m.attachment_id = a.id "
      "WHERE m.chat_id = ? "
      "ORDER BY m.timestamp DESC LIMIT 1");

    delete_message_stmt_ = db_->prepareStatement(
      "DELETE FROM messages WHERE id = ? AND sender_id = ?");

    update_message_stmt_ = db_->prepareStatement(
      "UPDATE messages SET content = ? WHERE id = ? AND sender_id = ?");

    get_message_history_stmt_ = db_->prepareStatement(
      "SELECT m.id, m.chat_id, m.sender_id, m.content, m.timestamp, m.attachment_id, "
      "       u.username, a.type, a.name, a.data "
      "FROM messages m "
      "JOIN users u ON m.sender_id = u.id "
      "LEFT JOIN attachments a ON m.attachment_id = a.id "
      "WHERE m.chat_id = ? AND m.timestamp BETWEEN ? AND ? "
      "ORDER BY m.timestamp ASC");

    search_messages_stmt_ = db_->prepareStatement(
      "SELECT m.id, m.chat_id, m.sender_id, m.content, m.timestamp, m.attachment_id, "
      "       u.username, a.type, a.name, a.data "
      "FROM messages m "
      "JOIN users u ON m.sender_id = u.id "
      "LEFT JOIN attachments a ON m.attachment_id = a.id "
      "JOIN chat_participants cp ON m.chat_id = cp.chat_id "
      "WHERE cp.user_id = ? AND m.content LIKE ? "
      "ORDER BY m.timestamp DESC LIMIT ?");

    get_chat_last_activity_stmt_ = db_->prepareStatement(
      "SELECT MAX(timestamp) FROM messages WHERE chat_id = ?");
  }

  ~MessageService() {
    sqlite3_finalize(get_undelivered_messages_stmt_);
    sqlite3_finalize(mark_message_read_stmt_);
    sqlite3_finalize(get_unread_count_stmt_);
    sqlite3_finalize(get_last_message_stmt_);
    sqlite3_finalize(delete_message_stmt_);
    sqlite3_finalize(update_message_stmt_);
    sqlite3_finalize(get_message_history_stmt_);
    sqlite3_finalize(search_messages_stmt_);
    sqlite3_finalize(get_chat_last_activity_stmt_);
  }

  std::vector<npchat::ChatMessage> getUndeliveredMessages(std::uint32_t user_id) {
    std::lock_guard<std::mutex> lock(mutex_);

    std::vector<npchat::ChatMessage> messages;

    sqlite3_bind_int(get_undelivered_messages_stmt_, 1, user_id);
    sqlite3_bind_int(get_undelivered_messages_stmt_, 2, user_id);

    while (sqlite3_step(get_undelivered_messages_stmt_) == SQLITE_ROW) {
      npchat::ChatMessage msg = buildMessageFromRow(get_undelivered_messages_stmt_);
      messages.push_back(std::move(msg));
    }

    sqlite3_reset(get_undelivered_messages_stmt_);
    return messages;
  }

  void markMessageAsRead(npchat::MessageId message_id, std::uint32_t user_id) {
    std::lock_guard<std::mutex> lock(mutex_);

    std::uint64_t timestamp = std::chrono::duration_cast<std::chrono::seconds>(
      std::chrono::system_clock::now().time_since_epoch()).count();

    sqlite3_bind_int(mark_message_read_stmt_, 1, message_id);
    sqlite3_bind_int(mark_message_read_stmt_, 2, user_id);
    sqlite3_bind_int64(mark_message_read_stmt_, 3, timestamp);

    sqlite3_step(mark_message_read_stmt_);
    sqlite3_reset(mark_message_read_stmt_);
  }

  std::uint32_t getUnreadMessageCount(std::uint32_t user_id) {
    std::lock_guard<std::mutex> lock(mutex_);

    sqlite3_bind_int(get_unread_count_stmt_, 1, user_id);
    sqlite3_bind_int(get_unread_count_stmt_, 2, user_id);
    sqlite3_bind_int(get_unread_count_stmt_, 3, user_id);

    std::uint32_t count = 0;
    if (sqlite3_step(get_unread_count_stmt_) == SQLITE_ROW) {
      count = sqlite3_column_int(get_unread_count_stmt_, 0);
    }

    sqlite3_reset(get_unread_count_stmt_);
    return count;
  }

  std::optional<npchat::ChatMessage> getLastMessage(npchat::ChatId chat_id) {
    std::lock_guard<std::mutex> lock(mutex_);

    sqlite3_bind_int(get_last_message_stmt_, 1, chat_id);

    if (sqlite3_step(get_last_message_stmt_) == SQLITE_ROW) {
      npchat::ChatMessage msg = buildMessageFromRow(get_last_message_stmt_);
      sqlite3_reset(get_last_message_stmt_);
      return msg;
    }

    sqlite3_reset(get_last_message_stmt_);
    return std::nullopt;
  }

  bool deleteMessage(npchat::MessageId message_id, std::uint32_t sender_id) {
    std::lock_guard<std::mutex> lock(mutex_);

    sqlite3_bind_int(delete_message_stmt_, 1, message_id);
    sqlite3_bind_int(delete_message_stmt_, 2, sender_id);

    bool success = (sqlite3_step(delete_message_stmt_) == SQLITE_DONE);
    sqlite3_reset(delete_message_stmt_);

    return success && sqlite3_changes(db_->getConnection()) > 0;
  }

  bool updateMessage(npchat::MessageId message_id, std::uint32_t sender_id, const std::string& new_content) {
    std::lock_guard<std::mutex> lock(mutex_);

    sqlite3_bind_text(update_message_stmt_, 1, new_content.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_int(update_message_stmt_, 2, message_id);
    sqlite3_bind_int(update_message_stmt_, 3, sender_id);

    bool success = (sqlite3_step(update_message_stmt_) == SQLITE_DONE);
    sqlite3_reset(update_message_stmt_);

    return success && sqlite3_changes(db_->getConnection()) > 0;
  }

  std::vector<npchat::ChatMessage> getMessageHistory(npchat::ChatId chat_id, std::uint64_t start_time, std::uint64_t end_time) {
    std::lock_guard<std::mutex> lock(mutex_);

    std::vector<npchat::ChatMessage> messages;

    sqlite3_bind_int(get_message_history_stmt_, 1, chat_id);
    sqlite3_bind_int64(get_message_history_stmt_, 2, start_time);
    sqlite3_bind_int64(get_message_history_stmt_, 3, end_time);

    while (sqlite3_step(get_message_history_stmt_) == SQLITE_ROW) {
      npchat::ChatMessage msg = buildMessageFromRow(get_message_history_stmt_);
      messages.push_back(std::move(msg));
    }

    sqlite3_reset(get_message_history_stmt_);
    return messages;
  }

  std::vector<npchat::ChatMessage> searchMessages(std::uint32_t user_id, const std::string& query, std::uint32_t limit = 50) {
    std::lock_guard<std::mutex> lock(mutex_);

    std::vector<npchat::ChatMessage> messages;

    std::string search_pattern = "%" + query + "%";

    sqlite3_bind_int(search_messages_stmt_, 1, user_id);
    sqlite3_bind_text(search_messages_stmt_, 2, search_pattern.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_int(search_messages_stmt_, 3, limit);

    while (sqlite3_step(search_messages_stmt_) == SQLITE_ROW) {
      npchat::ChatMessage msg = buildMessageFromRow(search_messages_stmt_);
      messages.push_back(std::move(msg));
    }

    sqlite3_reset(search_messages_stmt_);
    return messages;
  }

  std::uint64_t getChatLastActivity(npchat::ChatId chat_id) {
    std::lock_guard<std::mutex> lock(mutex_);

    sqlite3_bind_int(get_chat_last_activity_stmt_, 1, chat_id);

    std::uint64_t timestamp = 0;
    if (sqlite3_step(get_chat_last_activity_stmt_) == SQLITE_ROW) {
      timestamp = sqlite3_column_int64(get_chat_last_activity_stmt_, 0);
    }

    sqlite3_reset(get_chat_last_activity_stmt_);
    return timestamp;
  }

  // Real-time messaging functionality
  void setUserOnline(std::uint32_t user_id, std::function<void(const npchat::ChatMessage&)> callback) {
    std::lock_guard<std::mutex> lock(mutex_);
    online_users_.insert(user_id);
    delivery_callbacks_[user_id] = callback;
  }

  void setUserOffline(std::uint32_t user_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    online_users_.erase(user_id);
    delivery_callbacks_.erase(user_id);
  }

  bool isUserOnline(std::uint32_t user_id) const {
    std::lock_guard<std::mutex> lock(mutex_);
    return online_users_.find(user_id) != online_users_.end();
  }

  void deliverMessage(const npchat::ChatMessage& message, const std::vector<std::uint32_t>& recipients) {
    std::lock_guard<std::mutex> lock(mutex_);

    for (std::uint32_t recipient_id : recipients) {
      auto it = delivery_callbacks_.find(recipient_id);
      if (it != delivery_callbacks_.end()) {
        // User is online, deliver immediately
        try {
          it->second(message);
        } catch (const std::exception& e) {
          spdlog::error("Failed to deliver message to user {}: {}", recipient_id, e.what());
        }
      }
      // If user is offline, message will be delivered when they come online via getUndeliveredMessages
    }
  }

  // Batch operations for efficiency
  void markMultipleMessagesAsRead(const std::vector<npchat::MessageId>& message_ids, std::uint32_t user_id) {
    std::lock_guard<std::mutex> lock(mutex_);

    std::uint64_t timestamp = std::chrono::duration_cast<std::chrono::seconds>(
      std::chrono::system_clock::now().time_since_epoch()).count();

    // Begin transaction
    sqlite3_exec(db_->getConnection(), "BEGIN TRANSACTION", nullptr, nullptr, nullptr);

    for (npchat::MessageId message_id : message_ids) {
      sqlite3_bind_int(mark_message_read_stmt_, 1, message_id);
      sqlite3_bind_int(mark_message_read_stmt_, 2, user_id);
      sqlite3_bind_int64(mark_message_read_stmt_, 3, timestamp);

      sqlite3_step(mark_message_read_stmt_);
      sqlite3_reset(mark_message_read_stmt_);
    }

    // Commit transaction
    sqlite3_exec(db_->getConnection(), "COMMIT", nullptr, nullptr, nullptr);
  }

private:
  npchat::ChatMessage buildMessageFromRow(sqlite3_stmt* stmt) {
    npchat::ChatMessage msg;
    msg.chatId = sqlite3_column_int(stmt, 1);
    msg.timestamp = sqlite3_column_int64(stmt, 4);
    msg.content.text = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));

    // Handle attachment if present
    if (sqlite3_column_type(stmt, 5) != SQLITE_NULL) {
      npchat::ChatAttachment attachment;
      attachment.type = static_cast<npchat::ChatAttachmentType>(sqlite3_column_int(stmt, 7));
      attachment.name = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 8));

      const void* blob_data = sqlite3_column_blob(stmt, 9);
      int blob_size = sqlite3_column_bytes(stmt, 9);
      attachment.data.assign(
        static_cast<const std::uint8_t*>(blob_data),
        static_cast<const std::uint8_t*>(blob_data) + blob_size
      );

      msg.content.attachment = attachment;
    }

    return msg;
  }
};
