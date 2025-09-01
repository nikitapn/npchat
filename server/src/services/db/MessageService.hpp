#pragma once

#include <memory>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <functional>
#include <mutex>
#include <optional>
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
  explicit MessageService(const std::shared_ptr<Database>& database);
  ~MessageService();

  std::vector<npchat::ChatMessage> getUndeliveredMessages(std::uint32_t user_id);
  void markMessageAsRead(npchat::MessageId message_id, std::uint32_t user_id);
  std::uint32_t getUnreadMessageCount(std::uint32_t user_id);
  std::optional<npchat::ChatMessage> getLastMessage(npchat::ChatId chat_id);
  bool deleteMessage(npchat::MessageId message_id, std::uint32_t sender_id);
  bool updateMessage(npchat::MessageId message_id, std::uint32_t sender_id, const std::string& new_content);
  std::vector<npchat::ChatMessage> getMessageHistory(npchat::ChatId chat_id, std::uint64_t start_time, std::uint64_t end_time);
  std::vector<npchat::ChatMessage> searchMessages(std::uint32_t user_id, const std::string& query, std::uint32_t limit = 50);
  std::uint64_t getChatLastActivity(npchat::ChatId chat_id);

  // Real-time messaging functionality
  void setUserOnline(std::uint32_t user_id, std::function<void(const npchat::ChatMessage&)> callback);
  void setUserOffline(std::uint32_t user_id);
  bool isUserOnline(std::uint32_t user_id) const;
  void deliverMessage(const npchat::ChatMessage& message, const std::vector<std::uint32_t>& recipients);

  // Batch operations for efficiency
  void markMultipleMessagesAsRead(const std::vector<npchat::MessageId>& message_ids, std::uint32_t user_id);

private:
  npchat::ChatMessage buildMessageFromRow(sqlite3_stmt* stmt);
};
