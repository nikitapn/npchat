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
  explicit ChatService(const std::shared_ptr<Database>& database) : db_(database) {
    insert_message_stmt_ = db_->prepareStatement(
      "INSERT INTO messages (chat_id, sender_id, content, timestamp, attachment_id) VALUES (?, ?, ?, ?, ?)");

    get_messages_stmt_ = db_->prepareStatement(
      "SELECT m.id, m.chat_id, m.sender_id, m.content, m.timestamp, m.attachment_id, "
      "       u.username, a.type, a.name, a.data "
      "FROM messages m "
      "JOIN users u ON m.sender_id = u.id "
      "LEFT JOIN attachments a ON m.attachment_id = a.id "
      "WHERE m.chat_id = ? ORDER BY m.timestamp ASC LIMIT ? OFFSET ?");

    get_message_by_id_stmt_ = db_->prepareStatement(
      "SELECT m.id, m.chat_id, m.sender_id, m.content, m.timestamp, m.attachment_id, "
      "       u.username, a.type, a.name, a.data "
      "FROM messages m "
      "JOIN users u ON m.sender_id = u.id "
      "LEFT JOIN attachments a ON m.attachment_id = a.id "
      "WHERE m.id = ?");

    mark_delivered_stmt_ = db_->prepareStatement(
      "INSERT OR IGNORE INTO message_delivery (message_id, user_id, delivered_at) VALUES (?, ?, ?)");

    get_chat_participants_stmt_ = db_->prepareStatement(
      "SELECT user_id FROM chat_participants WHERE chat_id = ?");

    create_chat_stmt_ = db_->prepareStatement(
      "INSERT INTO chats (created_by, created_at) VALUES (?, ?)");

    add_participant_stmt_ = db_->prepareStatement(
      "INSERT INTO chat_participants (chat_id, user_id, joined_at) VALUES (?, ?, ?)");

    get_user_chats_stmt_ = db_->prepareStatement(
      "SELECT DISTINCT c.id, c.created_by, c.created_at "
      "FROM chats c "
      "JOIN chat_participants cp ON c.id = cp.chat_id "
      "WHERE cp.user_id = ?");

    insert_attachment_stmt_ = db_->prepareStatement(
      "INSERT INTO attachments (type, name, data) VALUES (?, ?, ?)");

    get_attachment_stmt_ = db_->prepareStatement(
      "SELECT type, name, data FROM attachments WHERE id = ?");

    find_existing_chat_stmt_ = db_->prepareStatement(
      "SELECT c.id FROM chats c "
      "JOIN chat_participants cp1 ON c.id = cp1.chat_id "
      "JOIN chat_participants cp2 ON c.id = cp2.chat_id "
      "WHERE cp1.user_id = ? AND cp2.user_id = ? AND cp1.user_id != cp2.user_id "
      "AND (SELECT COUNT(*) FROM chat_participants cp WHERE cp.chat_id = c.id) = 2 "
      "LIMIT 1");

    get_user_chats_details_stmt_ = db_->prepareStatement(
      "SELECT DISTINCT c.id, c.created_by, c.created_at, "
      "       (SELECT COUNT(*) FROM chat_participants cp WHERE cp.chat_id = c.id) as participant_count, "
      "       (SELECT MAX(m.timestamp) FROM messages m WHERE m.chat_id = c.id) as last_message_time "
      "FROM chats c "
      "JOIN chat_participants cp ON c.id = cp.chat_id "
      "WHERE cp.user_id = ? "
      "ORDER BY last_message_time DESC NULLS LAST");

    remove_participant_stmt_ = db_->prepareStatement(
      "DELETE FROM chat_participants WHERE chat_id = ? AND user_id = ?");

    delete_chat_stmt_ = db_->prepareStatement(
      "DELETE FROM chats WHERE id = ?");

    delete_chat_messages_stmt_ = db_->prepareStatement(
      "DELETE FROM messages WHERE chat_id = ?");
  }

  ~ChatService() {
    sqlite3_finalize(insert_message_stmt_);
    sqlite3_finalize(get_messages_stmt_);
    sqlite3_finalize(get_message_by_id_stmt_);
    sqlite3_finalize(mark_delivered_stmt_);
    sqlite3_finalize(get_chat_participants_stmt_);
    sqlite3_finalize(create_chat_stmt_);
    sqlite3_finalize(add_participant_stmt_);
    sqlite3_finalize(get_user_chats_stmt_);
    sqlite3_finalize(insert_attachment_stmt_);
    sqlite3_finalize(get_attachment_stmt_);
    sqlite3_finalize(find_existing_chat_stmt_);
    sqlite3_finalize(get_user_chats_details_stmt_);
    sqlite3_finalize(remove_participant_stmt_);
    sqlite3_finalize(delete_chat_stmt_);
    sqlite3_finalize(delete_chat_messages_stmt_);
  }

  std::uint32_t createChat(std::uint32_t creator_id, const std::vector<std::uint32_t>& participant_ids) {
    std::lock_guard<std::recursive_mutex> lock(mutex_);

    std::uint64_t timestamp = std::chrono::duration_cast<std::chrono::seconds>(
      std::chrono::system_clock::now().time_since_epoch()).count();

    // Create chat
    sqlite3_bind_int(create_chat_stmt_, 1, creator_id);
    sqlite3_bind_int64(create_chat_stmt_, 2, timestamp);

    if (sqlite3_step(create_chat_stmt_) != SQLITE_DONE) {
      sqlite3_reset(create_chat_stmt_);
      throw std::runtime_error("Failed to create chat");
    }

    std::uint32_t chat_id = static_cast<std::uint32_t>(sqlite3_last_insert_rowid(db_->getConnection()));
    sqlite3_reset(create_chat_stmt_);

    // Add creator as participant
    sqlite3_bind_int(add_participant_stmt_, 1, chat_id);
    sqlite3_bind_int(add_participant_stmt_, 2, creator_id);
    sqlite3_bind_int64(add_participant_stmt_, 3, timestamp);
    sqlite3_step(add_participant_stmt_);
    sqlite3_reset(add_participant_stmt_);

    // Add other participants
    for (std::uint32_t participant_id : participant_ids) {
      if (participant_id != creator_id) {
        sqlite3_bind_int(add_participant_stmt_, 1, chat_id);
        sqlite3_bind_int(add_participant_stmt_, 2, participant_id);
        sqlite3_bind_int64(add_participant_stmt_, 3, timestamp);
        sqlite3_step(add_participant_stmt_);
        sqlite3_reset(add_participant_stmt_);
      }
    }

    // Update cache
    std::vector<std::uint32_t> participants;
    participants.push_back(creator_id);
    for (auto id : participant_ids) {
      if (id != creator_id) {
        participants.push_back(id);
      }
    }
    chat_participants_cache_[chat_id] = std::move(participants);

    return chat_id;
  }

  npchat::MessageId sendMessage(std::uint32_t sender_id, npchat::ChatId chat_id, const npchat::ChatMessage& message) {
    std::lock_guard<std::recursive_mutex> lock(mutex_);

    // Verify sender is participant
    auto participants = getChatParticipants(chat_id);
    if (std::find(participants.begin(), participants.end(), sender_id) == participants.end()) {
      throw std::runtime_error("User is not a participant in this chat");
    }

    std::uint32_t attachment_id = 0;

    // Handle attachment if present
    if (message.attachment.has_value()) {
      const auto& attachment = message.attachment.value();

      sqlite3_bind_int(insert_attachment_stmt_, 1, static_cast<int>(attachment.type));
      sqlite3_bind_text(insert_attachment_stmt_, 2, attachment.name.c_str(), -1, SQLITE_STATIC);
      sqlite3_bind_blob(insert_attachment_stmt_, 3, attachment.data.data(), attachment.data.size(), SQLITE_STATIC);

      if (sqlite3_step(insert_attachment_stmt_) == SQLITE_DONE) {
        attachment_id = static_cast<std::uint32_t>(sqlite3_last_insert_rowid(db_->getConnection()));
      }
      sqlite3_reset(insert_attachment_stmt_);
    }

    // Insert message
    sqlite3_bind_int(insert_message_stmt_, 1, chat_id);
    sqlite3_bind_int(insert_message_stmt_, 2, sender_id);
    sqlite3_bind_text(insert_message_stmt_, 3, message.str.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_int64(insert_message_stmt_, 4, message.timestamp);
    if (attachment_id > 0) {
      sqlite3_bind_int(insert_message_stmt_, 5, attachment_id);
    } else {
      sqlite3_bind_null(insert_message_stmt_, 5);
    }

    if (sqlite3_step(insert_message_stmt_) != SQLITE_DONE) {
      sqlite3_reset(insert_message_stmt_);
      throw std::runtime_error("Failed to send message");
    }

    npchat::MessageId message_id = static_cast<npchat::MessageId>(sqlite3_last_insert_rowid(db_->getConnection()));
    sqlite3_reset(insert_message_stmt_);

    return message_id;
  }

  std::vector<npchat::ChatMessage> getMessages(npchat::ChatId chat_id, std::uint32_t limit = 50, std::uint32_t offset = 0) {
    std::lock_guard<std::recursive_mutex> lock(mutex_);

    std::vector<npchat::ChatMessage> messages;

    sqlite3_bind_int(get_messages_stmt_, 1, chat_id);
    sqlite3_bind_int(get_messages_stmt_, 2, limit);
    sqlite3_bind_int(get_messages_stmt_, 3, offset);

    while (sqlite3_step(get_messages_stmt_) == SQLITE_ROW) {
      npchat::ChatMessage msg;
      msg.chatId = sqlite3_column_int(get_messages_stmt_, 1);
      msg.timestamp = sqlite3_column_int64(get_messages_stmt_, 4);

      // Safely handle text content
      const char* content_text = reinterpret_cast<const char*>(sqlite3_column_text(get_messages_stmt_, 3));
      msg.str = content_text ? content_text : "";

      // Handle attachment if present
      if (sqlite3_column_type(get_messages_stmt_, 5) != SQLITE_NULL) {
        npchat::ChatAttachment attachment;
        attachment.type = static_cast<npchat::ChatAttachmentType>(sqlite3_column_int(get_messages_stmt_, 7));

        // Safely handle attachment name
        const char* attachment_name = reinterpret_cast<const char*>(sqlite3_column_text(get_messages_stmt_, 8));
        attachment.name = attachment_name ? attachment_name : "";

        // Safely handle blob data
        const void* blob_data = sqlite3_column_blob(get_messages_stmt_, 9);
        int blob_size = sqlite3_column_bytes(get_messages_stmt_, 9);
        if (blob_data && blob_size > 0) {
          attachment.data.assign(
            static_cast<const std::uint8_t*>(blob_data),
            static_cast<const std::uint8_t*>(blob_data) + blob_size
          );
        }

        msg.attachment = attachment;
      }

      messages.push_back(std::move(msg));
    }

    sqlite3_reset(get_messages_stmt_);
    return messages;
  }

  std::optional<npchat::ChatMessage> getMessageById(npchat::MessageId message_id) {
    std::lock_guard<std::recursive_mutex> lock(mutex_);

    sqlite3_bind_int(get_message_by_id_stmt_, 1, message_id);

    if (sqlite3_step(get_message_by_id_stmt_) == SQLITE_ROW) {
      npchat::ChatMessage msg;
      msg.chatId = sqlite3_column_int(get_message_by_id_stmt_, 1);
      msg.timestamp = sqlite3_column_int64(get_message_by_id_stmt_, 4);
      msg.str = reinterpret_cast<const char*>(sqlite3_column_text(get_message_by_id_stmt_, 3));

      // Handle attachment if present
      if (sqlite3_column_type(get_message_by_id_stmt_, 5) != SQLITE_NULL) {
        npchat::ChatAttachment attachment;
        attachment.type = static_cast<npchat::ChatAttachmentType>(sqlite3_column_int(get_message_by_id_stmt_, 7));
        attachment.name = reinterpret_cast<const char*>(sqlite3_column_text(get_message_by_id_stmt_, 8));

        const void* blob_data = sqlite3_column_blob(get_message_by_id_stmt_, 9);
        int blob_size = sqlite3_column_bytes(get_message_by_id_stmt_, 9);
        attachment.data.assign(
          static_cast<const std::uint8_t*>(blob_data),
          static_cast<const std::uint8_t*>(blob_data) + blob_size
        );

        msg.attachment = attachment;
      }

      sqlite3_reset(get_message_by_id_stmt_);
      return msg;
    }

    sqlite3_reset(get_message_by_id_stmt_);
    return std::nullopt;
  }

  void markMessageDelivered(npchat::MessageId message_id, std::uint32_t user_id) {
    std::lock_guard<std::recursive_mutex> lock(mutex_);

    std::uint64_t timestamp = std::chrono::duration_cast<std::chrono::seconds>(
      std::chrono::system_clock::now().time_since_epoch()).count();

    sqlite3_bind_int(mark_delivered_stmt_, 1, message_id);
    sqlite3_bind_int(mark_delivered_stmt_, 2, user_id);
    sqlite3_bind_int64(mark_delivered_stmt_, 3, timestamp);

    sqlite3_step(mark_delivered_stmt_);
    sqlite3_reset(mark_delivered_stmt_);
  }

  std::vector<std::uint32_t> getChatParticipants(npchat::ChatId chat_id) {
    // Check cache first
    auto it = chat_participants_cache_.find(chat_id);
    if (it != chat_participants_cache_.end()) {
      return it->second;
    }

    std::lock_guard<std::recursive_mutex> lock(mutex_);

    std::vector<std::uint32_t> participants;

    sqlite3_bind_int(get_chat_participants_stmt_, 1, chat_id);

    while (sqlite3_step(get_chat_participants_stmt_) == SQLITE_ROW) {
      participants.push_back(sqlite3_column_int(get_chat_participants_stmt_, 0));
    }

    sqlite3_reset(get_chat_participants_stmt_);

    // Update cache
    chat_participants_cache_[chat_id] = participants;

    return participants;
  }

  std::vector<npchat::ChatId> getUserChats(std::uint32_t user_id) {
    std::lock_guard<std::recursive_mutex> lock(mutex_);

    std::vector<npchat::ChatId> chats;

    sqlite3_bind_int(get_user_chats_stmt_, 1, user_id);

    while (sqlite3_step(get_user_chats_stmt_) == SQLITE_ROW) {
      chats.push_back(sqlite3_column_int(get_user_chats_stmt_, 0));
    }

    sqlite3_reset(get_user_chats_stmt_);
    return chats;
  }

  npchat::ChatList getUserChatsWithDetails(std::uint32_t user_id) {
    std::lock_guard<std::recursive_mutex> lock(mutex_);

    npchat::ChatList chats;

    sqlite3_bind_int(get_user_chats_details_stmt_, 1, user_id);

    while (sqlite3_step(get_user_chats_details_stmt_) == SQLITE_ROW) {
      npchat::Chat chat;
      chat.id = sqlite3_column_int(get_user_chats_details_stmt_, 0);
      chat.createdBy = sqlite3_column_int(get_user_chats_details_stmt_, 1);
      chat.createdAt = sqlite3_column_int(get_user_chats_details_stmt_, 2);
      chat.participantCount = sqlite3_column_int(get_user_chats_details_stmt_, 3);

      // Handle optional last message time
      if (sqlite3_column_type(get_user_chats_details_stmt_, 4) != SQLITE_NULL) {
        chat.lastMessageTime = sqlite3_column_int(get_user_chats_details_stmt_, 4);
      }

      chats.push_back(std::move(chat));
    }

    sqlite3_reset(get_user_chats_details_stmt_);
    return chats;
  }

  // Find existing chat between two users, or create a new one
  npchat::ChatId findOrCreateChatBetween(std::uint32_t user1_id, std::uint32_t user2_id) {
    std::lock_guard<std::recursive_mutex> lock(mutex_);

    // First, try to find existing chat between these two users
    sqlite3_bind_int(find_existing_chat_stmt_, 1, user1_id);
    sqlite3_bind_int(find_existing_chat_stmt_, 2, user2_id);

    if (sqlite3_step(find_existing_chat_stmt_) == SQLITE_ROW) {
      npchat::ChatId existing_chat_id = sqlite3_column_int(find_existing_chat_stmt_, 0);
      sqlite3_reset(find_existing_chat_stmt_);
      return existing_chat_id;
    }

    sqlite3_reset(find_existing_chat_stmt_);

    // No existing chat found, create a new one
    std::vector<std::uint32_t> participants = {user2_id};
    return createChat(user1_id, participants);
  }

  // Remove a participant from a chat
  bool removeParticipant(std::uint32_t requesting_user_id, npchat::ChatId chat_id, std::uint32_t participant_id) {
    std::lock_guard<std::recursive_mutex> lock(mutex_);

    // Get chat info to check creator
    sqlite3_bind_int(get_user_chats_details_stmt_, 1, requesting_user_id);

    bool is_chat_creator = false;
    bool is_participant = false;
    std::uint32_t chat_creator_id = 0;

    while (sqlite3_step(get_user_chats_details_stmt_) == SQLITE_ROW) {
      std::uint32_t current_chat_id = sqlite3_column_int(get_user_chats_details_stmt_, 0);
      if (current_chat_id == chat_id) {
        chat_creator_id = sqlite3_column_int(get_user_chats_details_stmt_, 1);
        is_chat_creator = (chat_creator_id == requesting_user_id);
        is_participant = true;
        break;
      }
    }
    sqlite3_reset(get_user_chats_details_stmt_);

    if (!is_participant) {
      throw std::runtime_error("User is not a participant in this chat");
    }

    // Authorization check:
    // - Chat creator can remove anyone
    // - Any participant can remove themselves
    if (!is_chat_creator && requesting_user_id != participant_id) {
      throw std::runtime_error("Only chat creator can remove other participants");
    }

    // Remove the participant
    sqlite3_bind_int(remove_participant_stmt_, 1, chat_id);
    sqlite3_bind_int(remove_participant_stmt_, 2, participant_id);

    bool success = (sqlite3_step(remove_participant_stmt_) == SQLITE_DONE);
    sqlite3_reset(remove_participant_stmt_);

    if (success) {
      // Update cache
      auto it = chat_participants_cache_.find(chat_id);
      if (it != chat_participants_cache_.end()) {
        auto& participants = it->second;
        participants.erase(std::remove(participants.begin(), participants.end(), participant_id), participants.end());

        // If no participants left, delete the entire chat
        if (participants.empty()) {
          deleteChat(chat_id);
          chat_participants_cache_.erase(it);
        }
      }
    }

    return success;
  }

  // Delete an entire chat (used when last participant leaves)
  bool deleteChat(npchat::ChatId chat_id) {
    std::lock_guard<std::recursive_mutex> lock(mutex_);

    // Delete all messages first (foreign key constraint)
    sqlite3_bind_int(delete_chat_messages_stmt_, 1, chat_id);
    sqlite3_step(delete_chat_messages_stmt_);
    sqlite3_reset(delete_chat_messages_stmt_);

    // Delete the chat
    sqlite3_bind_int(delete_chat_stmt_, 1, chat_id);
    bool success = (sqlite3_step(delete_chat_stmt_) == SQLITE_DONE);
    sqlite3_reset(delete_chat_stmt_);

    if (success) {
      // Remove from cache
      chat_participants_cache_.erase(chat_id);
    }

    return success;
  }

  // Get chat creator ID
  std::uint32_t getChatCreator(npchat::ChatId chat_id) {
    std::lock_guard<std::recursive_mutex> lock(mutex_);

    // We can reuse the get_user_chats_details_stmt but we need a specific query
    // For now, let's use a simple approach by getting participants and checking the creator field
    auto participants = getChatParticipants(chat_id);
    if (participants.empty()) {
      throw std::runtime_error("Chat not found or has no participants");
    }

    // Use the details query to get creator info
    sqlite3_stmt* get_chat_creator_stmt = db_->prepareStatement(
      "SELECT created_by FROM chats WHERE id = ?");

    sqlite3_bind_int(get_chat_creator_stmt, 1, chat_id);

    std::uint32_t creator_id = 0;
    if (sqlite3_step(get_chat_creator_stmt) == SQLITE_ROW) {
      creator_id = sqlite3_column_int(get_chat_creator_stmt, 0);
    }

    sqlite3_reset(get_chat_creator_stmt);
    sqlite3_finalize(get_chat_creator_stmt);

    if (creator_id == 0) {
      throw std::runtime_error("Chat not found");
    }

    return creator_id;
  }
};
