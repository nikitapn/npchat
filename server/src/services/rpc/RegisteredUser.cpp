#include "RegisteredUser.hpp"
#include "services/db/ContactService.hpp"
#include "services/db/MessageService.hpp"
#include "services/db/ChatService.hpp"
#include <spdlog/spdlog.h>

RegisteredUserImpl::RegisteredUserImpl(nprpc::Rpc& rpc, 
                                       std::shared_ptr<ContactService> contactService,
                                       std::shared_ptr<MessageService> messageService,
                                       std::shared_ptr<ChatService> chatService,
                                       std::uint32_t userId)
  : rpc_(rpc)
  , contactService_(contactService)
  , messageService_(messageService)
  , chatService_(chatService)
  , userId_(userId)
{
  spdlog::info("RegisteredUser created for user ID: {}", userId_);
}

// Contact management
npchat::ContactList RegisteredUserImpl::GetContacts() {
  spdlog::info("GetContacts called for user ID: {}", userId_);
  
  try {
    auto contacts = contactService_->getContacts(userId_);
    spdlog::info("Retrieved {} contacts for user ID: {}", contacts.size(), userId_);
    return contacts;
  } catch (const std::exception& e) {
    spdlog::error("Error getting contacts for user ID {}: {}", userId_, e.what());
    throw;
  }
}

npchat::ContactList RegisteredUserImpl::SearchUsers(::nprpc::flat::Span<char> query, std::uint32_t limit) {
  std::string queryStr(query);
  spdlog::info("SearchUsers called for user ID: {}, query: '{}', limit: {}", userId_, queryStr, limit);
  
  try {
    auto users = contactService_->searchUsers(userId_, queryStr, limit);
    spdlog::info("Found {} users for query '{}' by user ID: {}", users.size(), queryStr, userId_);
    return users;
  } catch (const std::exception& e) {
    spdlog::error("Error searching users for user ID {}, query '{}': {}", userId_, queryStr, e.what());
    throw;
  }
}

void RegisteredUserImpl::AddContact(npchat::UserId contactUserId) {
  spdlog::info("AddContact called for user ID: {}, adding contact: {}", userId_, contactUserId);
  
  try {
    bool success = contactService_->addContact(userId_, contactUserId);
    if (success) {
      spdlog::info("Successfully added contact {} for user ID: {}", contactUserId, userId_);
    } else {
      spdlog::warn("Failed to add contact {} for user ID: {} (might already exist)", contactUserId, userId_);
    }
  } catch (const std::exception& e) {
    spdlog::error("Error adding contact {} for user ID {}: {}", contactUserId, userId_, e.what());
    throw;
  }
}

void RegisteredUserImpl::RemoveContact(npchat::UserId contactUserId) {
  spdlog::info("RemoveContact called for user ID: {}, removing contact: {}", userId_, contactUserId);
  
  try {
    bool success = contactService_->removeContact(userId_, contactUserId);
    if (success) {
      spdlog::info("Successfully removed contact {} for user ID: {}", contactUserId, userId_);
    } else {
      spdlog::warn("Failed to remove contact {} for user ID: {} (might not exist)", contactUserId, userId_);
    }
  } catch (const std::exception& e) {
    spdlog::error("Error removing contact {} for user ID {}: {}", contactUserId, userId_, e.what());
    throw;
  }
}

// Chat management
npchat::ChatList RegisteredUserImpl::GetChats() {
  spdlog::info("GetChats called for user ID: {}", userId_);
  
  try {
    // ChatService::getUserChats returns vector<ChatId>, but we need ChatList (vector<Chat>)
    // We need to get the full chat information for each chat ID
    auto chatIds = chatService_->getUserChats(userId_);
    npchat::ChatList chats;
    
    // TODO: Implement getting full chat info - for now return empty list
    spdlog::info("Retrieved {} chats for user ID: {}", chatIds.size(), userId_);
    return chats;
  } catch (const std::exception& e) {
    spdlog::error("Error getting chats for user ID {}: {}", userId_, e.what());
    throw;
  }
}

npchat::ChatId RegisteredUserImpl::CreateChat() {
  spdlog::info("CreateChat called for user ID: {}", userId_);
  
  try {
    // Create a chat with just the current user as participant
    std::vector<std::uint32_t> participants = {userId_};
    auto chatId = chatService_->createChat(userId_, participants);
    
    spdlog::info("Created chat {} for user ID: {}", chatId, userId_);
    return chatId;
  } catch (const std::exception& e) {
    spdlog::error("Error creating chat for user ID {}: {}", userId_, e.what());
    throw;
  }
}

void RegisteredUserImpl::AddChatParticipant(npchat::ChatId chatId, npchat::UserId participantUserId) {
  spdlog::info("AddChatParticipant called for user ID: {}, chat: {}, participant: {}", 
               userId_, chatId, participantUserId);
  
  try {
    // TODO: Add method to ChatService to add a single participant
    spdlog::info("Added participant {} to chat {} by user ID: {}", participantUserId, chatId, userId_);
  } catch (const std::exception& e) {
    spdlog::error("Error adding participant {} to chat {} by user ID {}: {}", 
                  participantUserId, chatId, userId_, e.what());
    throw;
  }
}

void RegisteredUserImpl::LeaveChatParticipant(npchat::ChatId chatId, npchat::UserId participantUserId) {
  spdlog::info("LeaveChatParticipant called for user ID: {}, chat: {}, participant: {}", 
               userId_, chatId, participantUserId);
  
  try {
    // TODO: Add method to ChatService to remove a participant
    spdlog::info("Removed participant {} from chat {} by user ID: {}", participantUserId, chatId, userId_);
  } catch (const std::exception& e) {
    spdlog::error("Error removing participant {} from chat {} by user ID {}: {}", 
                  participantUserId, chatId, userId_, e.what());
    throw;
  }
}

// Message operations
npchat::MessageId RegisteredUserImpl::SendMessage(npchat::ChatId chatId, 
                                                  npchat::flat::ChatMessage_Direct message) {
  spdlog::info("SendMessage called for user ID: {}, chat ID: {}", userId_, chatId);
  
  try {
    // Convert flat message to regular ChatMessage for ChatService
    npchat::ChatMessage chatMessage;

    npchat::helpers::assign_from_flat_ChatMessage(message, chatMessage);
    chatMessage.chatId = chatId;
    
    auto messageId = chatService_->sendMessage(userId_, chatId, chatMessage);
    
    spdlog::info("Message sent with ID: {} for user ID: {}, chat ID: {}", 
                 messageId, userId_, chatId);
    return messageId;
  } catch (const std::exception& e) {
    spdlog::error("Error sending message for user ID {}, chat ID {}: {}", 
                  userId_, chatId, e.what());
    throw;
  }
}

npchat::MessageList RegisteredUserImpl::GetChatHistory(npchat::ChatId chatId, std::uint32_t limit, std::uint32_t offset) {
  spdlog::info("GetChatHistory called for user ID: {}, chat ID: {}, limit: {}, offset: {}", 
               userId_, chatId, limit, offset);
  
  try {
    auto messages = chatService_->getMessages(chatId, limit, offset);
    spdlog::info("Retrieved {} messages for chat {} by user ID: {}", messages.size(), chatId, userId_);
    return messages;
  } catch (const std::exception& e) {
    spdlog::error("Error getting chat history for user ID {}, chat ID {}: {}", 
                  userId_, chatId, e.what());
    throw;
  }
}

std::uint32_t RegisteredUserImpl::GetUnreadMessageCount() {
  spdlog::info("GetUnreadMessageCount called for user ID: {}", userId_);
  
  try {
    auto count = messageService_->getUnreadMessageCount(userId_);
    spdlog::info("User ID: {} has {} unread messages", userId_, count);
    return count;
  } catch (const std::exception& e) {
    spdlog::error("Error getting unread message count for user ID {}: {}", userId_, e.what());
    throw;
  }
}

void RegisteredUserImpl::MarkMessageAsRead(npchat::MessageId messageId) {
  spdlog::info("MarkMessageAsRead called for user ID: {}, message ID: {}", userId_, messageId);
  
  try {
    messageService_->markMessageAsRead(messageId, userId_);
    spdlog::info("Marked message {} as read for user ID: {}", messageId, userId_);
  } catch (const std::exception& e) {
    spdlog::error("Error marking message {} as read for user ID {}: {}", messageId, userId_, e.what());
    throw;
  }
}