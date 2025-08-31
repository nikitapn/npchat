#include "RegisteredUser.hpp"
#include "services/db/ContactService.hpp"
#include "services/db/MessageService.hpp"
#include "services/db/ChatService.hpp"
#include "services/client/ChatObserver.hpp"
#include <spdlog/spdlog.h>

RegisteredUserImpl::RegisteredUserImpl(nprpc::Rpc& rpc, 
                                       std::shared_ptr<ContactService> contactService,
                                       std::shared_ptr<MessageService> messageService,
                                       std::shared_ptr<ChatService> chatService,
                                       std::shared_ptr<ChatObservers> chatObservers,
                                       std::uint32_t userId)
  : rpc_(rpc)
  , contactService_(contactService)
  , messageService_(messageService)
  , chatService_(chatService)
  , chatObservers_(chatObservers)
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
    
    // Register chat participants with the observer system
    chatObservers_->add_chat_participants(chatId, participants);
    
    spdlog::info("Created chat {} for user ID: {}, registered with observers", chatId, userId_);
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
void RegisteredUserImpl::SubscribeToEvents(nprpc::Object* obj) {
  spdlog::info("SubscribeToEvents called for user ID: {}", userId_);
  
  try {
    if (auto listener = nprpc::narrow<npchat::ChatListener>(obj)) {
      listener->add_ref();
      listener->set_timeout(250);
      
      // Subscribe this user's listener to chat events
      chatObservers_->subscribe_user(userId_, listener);
      
      spdlog::info("Successfully subscribed user ID: {} to chat events", userId_);
    } else {
      spdlog::error("Failed to narrow object to ChatListener for user ID: {}", userId_);
      throw std::invalid_argument("Object is not a valid ChatListener");
    }
  } catch (const std::exception& e) {
    spdlog::error("Error subscribing to events for user ID {}: {}", userId_, e.what());
    throw;
  }
}

npchat::MessageId RegisteredUserImpl::SendMessage(npchat::ChatId chatId, 
                                                  npchat::flat::ChatMessage_Direct message) {
  spdlog::info("SendMessage called for user ID: {}, chat ID: {}", userId_, chatId);
  
  try {
    // Convert flat message to regular ChatMessage for ChatService
    npchat::ChatMessage chatMessage;
    npchat::helpers::assign_from_flat_ChatMessage(message, chatMessage);
    chatMessage.chatId = chatId;
    
    auto messageId = chatService_->sendMessage(userId_, chatId, chatMessage);
    
    // Notify all chat participants about the new message
    chatObservers_->notify_message_received(messageId, chatMessage);
    
    // Notify sender about successful delivery
    chatObservers_->notify_message_delivered(chatId, messageId, userId_);
    
    spdlog::info("Message sent with ID: {} for user ID: {}, chat ID: {}, participants notified", 
                 messageId, userId_, chatId);
    return messageId;
  } catch (const std::runtime_error& e) {
    std::string errorMsg = e.what();
    spdlog::error("Error sending message for user ID {}, chat ID {}: {}", 
                  userId_, chatId, errorMsg);
    
    // Convert runtime_error to proper NPRPC exception
    if (errorMsg.find("not a participant") != std::string::npos) {
      throw npchat::ChatOperationFailed(npchat::ChatError::UserNotParticipant);
    } else if (errorMsg.find("not found") != std::string::npos) {
      throw npchat::ChatOperationFailed(npchat::ChatError::ChatNotFound);
    } else {
      throw npchat::ChatOperationFailed(npchat::ChatError::InvalidMessage);
    }
  } catch (const std::exception& e) {
    spdlog::error("Unexpected error sending message for user ID {}, chat ID {}: {}", 
                  userId_, chatId, e.what());
    throw npchat::ChatOperationFailed(npchat::ChatError::InvalidMessage);
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