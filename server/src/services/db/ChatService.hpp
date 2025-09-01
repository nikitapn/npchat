#pragma once

#include <memory>
#include <vector>
#include <unordered_map>
#include <mutex>
#include <sqlite3.h>
#include <spdlog/spdlog.h>
#include "Database.hpp"
#include "npchat_stub/npchat.hpp"

class ChatService {
private:
  std::shared_ptr<Database> db_;
  mutable std::recursive_mutex mutex_;

  // Prepared statements
  sqlite3_stmt* insert_message_stmt_;
  sqlite3_stmt* get_messages_stmt_;
  sqlite3_stmt* get_message_by_id_stmt_;
  sqlite3_stmt* mark_delivered_stmt_;
  sqlite3_stmt* get_chat_participants_stmt_;
  sqlite3_stmt* create_chat_stmt_;
  sqlite3_stmt* add_participant_stmt_;
  sqlite3_stmt* get_user_chats_stmt_;
  sqlite3_stmt* insert_attachment_stmt_;
  sqlite3_stmt* get_attachment_stmt_;
  sqlite3_stmt* find_existing_chat_stmt_;
  sqlite3_stmt* get_user_chats_details_stmt_;
  sqlite3_stmt* remove_participant_stmt_;
  sqlite3_stmt* delete_chat_stmt_;
  sqlite3_stmt* delete_chat_messages_stmt_;

  std::unordered_map<std::uint32_t, std::vector<std::uint32_t>> chat_participants_cache_;

public:
  explicit ChatService(const std::shared_ptr<Database>& database);
  ~ChatService();

  std::uint32_t createChat(std::uint32_t creator_id, const std::vector<std::uint32_t>& participant_ids);

  npchat::MessageId sendMessage(std::uint32_t sender_id, npchat::ChatId chat_id, const npchat::ChatMessageContent& content);

  std::vector<npchat::ChatMessage> getMessages(npchat::ChatId chat_id, std::uint32_t limit = 50, std::uint32_t offset = 0);

  std::optional<npchat::ChatMessage> getMessageById(npchat::MessageId message_id);

  void markMessageDelivered(npchat::MessageId message_id, std::uint32_t user_id);

  std::vector<std::uint32_t> getChatParticipants(npchat::ChatId chat_id);

  std::vector<npchat::ChatId> getUserChats(std::uint32_t user_id);

  npchat::ChatList getUserChatsWithDetails(std::uint32_t user_id);

  // Find existing chat between two users, or create a new one
  npchat::ChatId findOrCreateChatBetween(std::uint32_t user1_id, std::uint32_t user2_id);

  // Remove a participant from a chat
  bool removeParticipant(std::uint32_t requesting_user_id, npchat::ChatId chat_id, std::uint32_t participant_id);

  // Delete an entire chat (used when last participant leaves)
  bool deleteChat(npchat::ChatId chat_id);

  // Get chat creator ID
  std::uint32_t getChatCreator(npchat::ChatId chat_id);
};
