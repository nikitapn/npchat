#include "RegisteredUser.hpp"
#include "services/db/ContactService.hpp"
#include "services/db/MessageService.hpp"
#include "services/db/ChatService.hpp"
#include "services/db/AuthService.hpp"
#include "services/client/ChatObserver.hpp"
#include <spdlog/spdlog.h>
#include <algorithm>

RegisteredUserImpl::RegisteredUserImpl(nprpc::Rpc& rpc,
                                       std::shared_ptr<ContactService> contactService,
                                       std::shared_ptr<MessageService> messageService,
                                       std::shared_ptr<ChatService> chatService,
                                       std::shared_ptr<ChatObservers> chatObservers,
                                       std::shared_ptr<AuthService> authService,
                                       std::shared_ptr<WebRTCService> webrtcService,
                                       std::uint32_t userId)
  : rpc_(rpc)
  , contactService_(contactService)
  , messageService_(messageService)
  , chatService_(chatService)
  , chatObservers_(chatObservers)
  , authService_(authService)
  , webrtcService_(webrtcService)
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

npchat::Contact RegisteredUserImpl::GetCurrentUser() {
  spdlog::info("GetCurrentUser called for user ID: {}", userId_);

  try {
    auto user = authService_->getUserById(userId_);
    if (user) {
      spdlog::info("Retrieved current user info for user ID: {}", userId_);
      return *user;
    } else {
      spdlog::error("User not found for ID: {}", userId_);
      throw npchat::ChatOperationFailed{npchat::ChatError::UserNotParticipant};
    }
  } catch (const std::exception& e) {
    spdlog::error("Error getting current user for ID {}: {}", userId_, e.what());
    throw;
  }
}

npchat::Contact RegisteredUserImpl::GetUserById(npchat::UserId userId) {
  spdlog::info("GetUserById called for user ID: {} by user ID: {}", userId, userId_);

  try {
    auto user = authService_->getUserById(userId);
    if (user) {
      spdlog::info("Retrieved user info for user ID: {}", userId);
      return *user;
    } else {
      spdlog::error("User not found for ID: {}", userId);
      throw npchat::AuthorizationFailed{npchat::AuthorizationError::AccessDenied};
    }
  } catch (const std::exception& e) {
    spdlog::error("Error getting user for ID {}: {}", userId, e.what());
    throw;
  }
}

// Chat management
npchat::ChatList RegisteredUserImpl::GetChats() {
  spdlog::info("GetChats called for user ID: {}", userId_);

  try {
    // Use the new method that returns full chat details
    auto chats = chatService_->getUserChatsWithDetails(userId_);
    spdlog::info("Retrieved {} chats for user ID: {}", chats.size(), userId_);
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

npchat::ChatId RegisteredUserImpl::CreateChatWith(npchat::UserId otherUserId) {
  spdlog::info("CreateChatWith called for user ID: {} with user: {}", userId_, otherUserId);

  try {
    // Find existing chat or create a new one
    auto chatId = chatService_->findOrCreateChatBetween(userId_, otherUserId);

    // Get the participants to register with observer system
    auto participants = chatService_->getChatParticipants(chatId);
    chatObservers_->add_chat_participants(chatId, participants);

    spdlog::info("Found/created chat {} between user {} and user {}, registered with observers",
                 chatId, userId_, otherUserId);
    return chatId;
  } catch (const std::exception& e) {
    spdlog::error("Error creating chat between user {} and user {}: {}", userId_, otherUserId, e.what());
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
    // Use ChatService to remove the participant with proper authorization
    bool success = chatService_->removeParticipant(userId_, chatId, participantUserId);

    if (success) {
      spdlog::info("Successfully removed participant {} from chat {} by user ID: {}",
                   participantUserId, chatId, userId_);
      // Also update observers mapping so future notifications respect current participants
      chatObservers_->remove_chat_participant(chatId, participantUserId);
    } else {
      spdlog::warn("Failed to remove participant {} from chat {} by user ID: {}",
                   participantUserId, chatId, userId_);
    }
  } catch (const std::runtime_error& e) {
    std::string errorMsg = e.what();
    spdlog::error("Error removing participant {} from chat {} by user ID {}: {}",
                  participantUserId, chatId, userId_, errorMsg);

    // Convert runtime errors to proper NPRPC exceptions
    if (errorMsg.find("not a participant") != std::string::npos) {
      throw npchat::ChatOperationFailed(npchat::ChatError::UserNotParticipant);
    } else if (errorMsg.find("not found") != std::string::npos || errorMsg.find("Chat not found") != std::string::npos) {
      throw npchat::ChatOperationFailed(npchat::ChatError::ChatNotFound);
    } else if (errorMsg.find("Only chat creator") != std::string::npos) {
      throw npchat::ChatOperationFailed(npchat::ChatError::UserNotParticipant); // Closest error for "not authorized"
    } else {
      throw npchat::ChatOperationFailed(npchat::ChatError::InvalidMessage);
    }
  } catch (const std::exception& e) {
    spdlog::error("Unexpected error removing participant {} from chat {} by user ID {}: {}",
                  participantUserId, chatId, userId_, e.what());
    throw npchat::ChatOperationFailed(npchat::ChatError::InvalidMessage);
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

      // Prime observer with all existing chats and their participants so
      // messages from existing chats are delivered even if the chat wasn't just created.
      try {
        auto chats = chatService_->getUserChatsWithDetails(userId_);
        for (const auto& chat : chats) {
          // Fetch participants for each chat and register them in the observer mapping
          auto participants = chatService_->getChatParticipants(chat.id);
          chatObservers_->add_chat_participants(chat.id, participants);
        }
        spdlog::info("Registered {} existing chats for user {} in observers", chats.size(), userId_);
      } catch (const std::exception& e) {
        spdlog::warn("Failed to pre-register existing chats for user {}: {}", userId_, e.what());
      }

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
                                                  npchat::flat::ChatMessageContent_Direct content) {
  spdlog::info("SendMessage called for user ID: {}, chat ID: {}", userId_, chatId);

  try {
    // Convert flat message to regular ChatMessage for ChatService
    npchat::ChatMessageContent messageContent;
    npchat::helpers::assign_from_flat_ChatMessageContent(content, messageContent);

    auto messageId = chatService_->sendMessage(userId_, chatId, messageContent);

    // Notify all chat participants about the new message
    npchat::ChatMessage chatMessage {
      .messageId = messageId,
      .senderId = userId_,
      .chatId = chatId,
      .content = std::move(messageContent)
    };
    chatObservers_->notify_message_received(messageId, chatMessage, userId_);

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
    // First, verify that the user is a participant in this chat
    auto participants = chatService_->getChatParticipants(chatId);
    bool isParticipant = std::find(participants.begin(), participants.end(), userId_) != participants.end();

    if (!isParticipant) {
      spdlog::warn("User {} attempted to access chat history for chat {} without being a participant",
                   userId_, chatId);
      throw npchat::ChatOperationFailed(npchat::ChatError::UserNotParticipant);
    }

    auto messages = chatService_->getMessages(chatId, limit, offset);
    spdlog::info("Retrieved {} messages for chat {} by user ID: {}", messages.size(), chatId, userId_);
    return messages;
  } catch (const npchat::ChatOperationFailed&) {
    // Re-throw NPRPC exceptions as-is
    throw;
  } catch (const std::exception& e) {
    spdlog::error("Error getting chat history for user ID {}, chat ID {}: {}",
                  userId_, chatId, e.what());
    throw npchat::ChatOperationFailed(npchat::ChatError::ChatNotFound);
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

// WebRTC video calling
std::string RegisteredUserImpl::InitiateCall(npchat::ChatId chatId, ::nprpc::flat::Span<char> offer) {
  spdlog::info("InitiateCall called for user ID: {}, chat ID: {}", userId_, chatId);

  try {
    // Check if user is a participant in the chat
    auto chatParticipants = chatService_->getChatParticipants(chatId);
    bool isParticipant = std::find(chatParticipants.begin(), chatParticipants.end(), userId_) != chatParticipants.end();

    if (!isParticipant) {
      spdlog::error("User {} is not a participant in chat {}", userId_, chatId);
      throw npchat::ChatOperationFailed{npchat::ChatError::UserNotParticipant};
    }

    // Find the other participant (assuming 1-on-1 chat for now)
    npchat::UserId otherUserId = 0;
    for (auto participantId : chatParticipants) {
      if (participantId != userId_) {
        otherUserId = participantId;
        break;
      }
    }

    if (otherUserId == 0) {
      spdlog::error("Could not find other participant in chat {}", chatId);
      throw npchat::ChatOperationFailed{npchat::ChatError::ChatNotFound};
    }

    // Check if there's already an active call in this chat
    auto activeCalls = webrtcService_->getActiveCallsForChat(chatId);
    if (!activeCalls.empty()) {
      spdlog::error("Call already active in chat {}", chatId);
      throw npchat::ChatOperationFailed{npchat::ChatError::InvalidMessage};
    }

    std::string callId = webrtcService_->initiateCall(chatId, userId_, otherUserId, offer);

    // Notify all chat participants about the call initiation
    chatObservers_->notify_call_initiated(callId, chatId, userId_, otherUserId, offer);

    spdlog::info("Call initiated: {} in chat {} from {} to {}", callId, chatId, userId_, otherUserId);
    return callId;
  } catch (const std::exception& e) {
    spdlog::error("Error initiating call for user ID {} in chat {}: {}", userId_, chatId, e.what());
    throw;
  }
}

void RegisteredUserImpl::AnswerCall(::nprpc::flat::Span<char> callId, ::nprpc::flat::Span<char> answer) {
  auto callIdStr = (std::string_view)callId;
  auto answerStr = (std::string_view)answer;

  spdlog::info("AnswerCall called for user ID: {}, call ID: {}", userId_, callIdStr);

  try {
    auto callOpt = webrtcService_->getCall(callIdStr);
    if (!callOpt) {
      spdlog::error("Call not found: {}", callIdStr);
      throw npchat::ChatOperationFailed{npchat::ChatError::ChatNotFound};
    }

    const auto& call = *callOpt;

    // Verify that this user is the callee
    if (call.calleeId != userId_) {
      spdlog::error("User {} is not authorized to answer call {}", userId_, callIdStr);
      throw npchat::ChatOperationFailed{npchat::ChatError::UserNotParticipant};
    }

    if (!webrtcService_->answerCall(callIdStr, answerStr)) {
      spdlog::error("Failed to answer call: {}", callIdStr);
      throw npchat::ChatOperationFailed{npchat::ChatError::InvalidMessage};
    }

    // Notify the caller about the answer
    chatObservers_->notify_call_answered(callIdStr, answerStr, call.callerId);

    spdlog::info("Call answered: {}", callIdStr);
  } catch (const std::exception& e) {
    spdlog::error("Error answering call {} for user ID {}: {}", callIdStr, userId_, e.what());
    throw;
  }
}

void RegisteredUserImpl::SendIceCandidate(::nprpc::flat::Span<char> callId, ::nprpc::flat::Span<char> candidate) {
  auto callIdStr = (std::string_view)callId;
  auto candidateStr = (std::string_view)candidate;

  spdlog::info("SendIceCandidate called for user ID: {}, call ID: {}", userId_, callIdStr);

  try {
    auto callOpt = webrtcService_->getCall(callIdStr);
    if (!callOpt) {
      spdlog::error("Call not found: {}", callIdStr);
      throw npchat::ChatOperationFailed{npchat::ChatError::ChatNotFound};
    }

    const auto& call = *callOpt;

    // Verify that this user is a participant in the call
    if (call.callerId != userId_ && call.calleeId != userId_) {
      spdlog::error("User {} is not a participant in call {}", userId_, callIdStr);
      throw npchat::ChatOperationFailed{npchat::ChatError::UserNotParticipant};
    }

    if (!webrtcService_->addIceCandidate(callIdStr, candidateStr)) {
      spdlog::error("Failed to add ICE candidate to call: {}", callIdStr);
      throw npchat::ChatOperationFailed{npchat::ChatError::InvalidMessage};
    }

    // Determine the target user (the other participant)
    npchat::UserId targetUserId = (call.callerId == userId_) ? call.calleeId : call.callerId;

    // Notify the other participant about the ICE candidate
    chatObservers_->notify_ice_candidate(callIdStr, candidateStr, targetUserId);

    spdlog::info("ICE candidate sent for call: {}", callIdStr);
  } catch (const std::exception& e) {
    spdlog::error("Error sending ICE candidate for call {} by user ID {}: {}", callIdStr, userId_, e.what());
    throw;
  }
}

void RegisteredUserImpl::EndCall(::nprpc::flat::Span<char> callId) {
  auto callIdStr = (std::string_view)callId;

  spdlog::info("EndCall called for user ID: {}, call ID: {}", userId_, callIdStr);

  try {
    auto callOpt = webrtcService_->getCall(callIdStr);
    if (!callOpt) {
      spdlog::error("Call not found: {}", callIdStr);
      throw npchat::ChatOperationFailed{npchat::ChatError::ChatNotFound};
    }

    const auto& call = *callOpt;

    // Verify that this user is a participant in the call
    if (call.callerId != userId_ && call.calleeId != userId_) {
      spdlog::error("User {} is not a participant in call {}", userId_, callIdStr);
      throw npchat::ChatOperationFailed{npchat::ChatError::UserNotParticipant};
    }

    if (!webrtcService_->endCall(callIdStr)) {
      spdlog::error("Failed to end call: {}", callIdStr);
      throw npchat::ChatOperationFailed{npchat::ChatError::InvalidMessage};
    }

    // Notify all participants about the call ending
    chatObservers_->notify_call_ended(callIdStr, "ended", call.chatId);

    spdlog::info("Call ended: {}", callIdStr);
  } catch (const std::exception& e) {
    spdlog::error("Error ending call {} by user ID {}: {}", callIdStr, userId_, e.what());
    throw;
  }
}