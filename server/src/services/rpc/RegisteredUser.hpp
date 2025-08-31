#pragma once

#include "npchat_stub/npchat.hpp"

class ContactService;
class MessageService;
class ChatService;

class RegisteredUserImpl : public npchat::IRegisteredUser_Servant {
  nprpc::Rpc& rpc_;
  std::shared_ptr<ContactService> contactService_;
  std::shared_ptr<MessageService> messageService_;
  std::shared_ptr<ChatService> chatService_;
  std::uint32_t userId_;
  
public:
  RegisteredUserImpl(nprpc::Rpc& rpc, 
                     std::shared_ptr<ContactService> contactService,
                     std::shared_ptr<MessageService> messageService,
                     std::shared_ptr<ChatService> chatService,
                     std::uint32_t userId);

  // Contact management
  virtual npchat::ContactList GetContacts() override;
  virtual npchat::ContactList SearchUsers(::nprpc::flat::Span<char> query, std::uint32_t limit) override;
  virtual void AddContact(npchat::UserId userId) override;
  virtual void RemoveContact(npchat::UserId userId) override;
  
  // Chat management  
  virtual npchat::ChatList GetChats() override;
  virtual npchat::ChatId CreateChat() override;
  virtual void AddChatParticipant(npchat::ChatId chatId, npchat::UserId userId) override;
  virtual void LeaveChatParticipant(npchat::ChatId chatId, npchat::UserId userId) override;
  
  // Message operations
  virtual npchat::MessageId SendMessage(npchat::ChatId chatId, 
                                        npchat::flat::ChatMessage_Direct message) override;
  virtual npchat::MessageList GetChatHistory(npchat::ChatId chatId, std::uint32_t limit, std::uint32_t offset) override;
  virtual std::uint32_t GetUnreadMessageCount() override;
  virtual void MarkMessageAsRead(npchat::MessageId messageId) override;
};