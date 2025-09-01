#pragma once

#include "npchat_stub/npchat.hpp"
class AuthService;
class ContactService;
class MessageService;
class ChatService;
class ChatObservers;

class AuthorizatorImpl : public npchat::IAuthorizator_Servant {
  nprpc::Rpc& rpc_;
  std::shared_ptr<AuthService> authService_;
  std::shared_ptr<ContactService> contactService_;
  std::shared_ptr<MessageService> messageService_;
  std::shared_ptr<ChatService> chatService_;
  std::shared_ptr<ChatObservers> chatObservers_;
  nprpc::Poa* user_poa_;

public:
  AuthorizatorImpl(nprpc::Rpc& rpc,
                   std::shared_ptr<AuthService> authService,
                   std::shared_ptr<ContactService> contactService,
                   std::shared_ptr<MessageService> messageService,
                   std::shared_ptr<ChatService> chatService,
                   std::shared_ptr<ChatObservers> chatObservers);

  virtual npchat::UserData LogIn (::nprpc::flat::Span<char> login, ::nprpc::flat::Span<char> password) override;

  virtual npchat::UserData LogInWithSessionId (::nprpc::flat::Span<char> session_id) override;

  virtual bool LogOut (::nprpc::flat::Span<char> session_id) override;

  virtual bool CheckUsername (::nprpc::flat::Span<char> username) override;

  virtual bool CheckEmail (::nprpc::flat::Span<char> email) override;

  virtual void RegisterStepOne (::nprpc::flat::Span<char> username, ::nprpc::flat::Span<char> email, ::nprpc::flat::Span<char> password) override;

  virtual void RegisterStepTwo (::nprpc::flat::Span<char> username, uint32_t code) override;
};