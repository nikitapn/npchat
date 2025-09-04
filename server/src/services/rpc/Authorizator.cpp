#include "Authorizator.hpp"
#include "RegisteredUser.hpp"
#include "services/db/AuthService.hpp"
#include "services/db/ContactService.hpp"
#include "services/db/MessageService.hpp"
#include "services/db/ChatService.hpp"
#include "services/client/ChatObserver.hpp"

AuthorizatorImpl::AuthorizatorImpl(nprpc::Rpc& rpc,
                                   std::shared_ptr<AuthService> authService,
                                   std::shared_ptr<ContactService> contactService,
                                   std::shared_ptr<MessageService> messageService,
                                   std::shared_ptr<ChatService> chatService,
                                   std::shared_ptr<ChatObservers> chatObservers)
  : rpc_(rpc)
  , authService_(authService)
  , contactService_(contactService)
  , messageService_(messageService)
  , chatService_(chatService)
  , chatObservers_(chatObservers)
{
  // Create POA for user objects (RegisteredUser instances)
  user_poa_ = nprpc::PoaBuilder(&rpc)
    .with_max_objects(1024)
    .with_lifespan(nprpc::PoaPolicy::Lifespan::Transient)
    .build();
}

npchat::UserData AuthorizatorImpl::LogIn (::nprpc::flat::Span<char> login, ::nprpc::flat::Span<char> password) {
  auto userData = authService_->logIn(std::string_view(login), std::string_view(password));

  // Get user ID from the login
  std::uint32_t userId = authService_->getUserIdFromLogin(std::string_view(login));

  // Create a new RegisteredUser object
  auto registeredUser = new RegisteredUserImpl(rpc_, contactService_, messageService_,
                                               chatService_, chatObservers_, authService_, userId);

  // Activate the object with the POA
  auto oid = user_poa_->activate_object(registeredUser,
    nprpc::ObjectActivationFlags::SESSION_SPECIFIC,
    &nprpc::get_context()
  );

  // Set the registeredUser field to the activated object
  userData.registeredUser = oid;

  return userData;
}

npchat::UserData AuthorizatorImpl::LogInWithSessionId (::nprpc::flat::Span<char> session_id) {
  auto userData = authService_->logInWithSessionId(std::string_view(session_id));

  // Get user ID from the session
  std::uint32_t userId = authService_->getUserIdFromSession(std::string_view(session_id));

  // Create a new RegisteredUser object
  auto registeredUser = new RegisteredUserImpl(rpc_, contactService_, messageService_,
                                               chatService_, chatObservers_, authService_, userId);

  // Activate the object with the POA
  auto oid = user_poa_->activate_object(registeredUser,
    nprpc::ObjectActivationFlags::SESSION_SPECIFIC,
    &nprpc::get_context()
  );

  // Set the registeredUser field to the activated object
  userData.registeredUser = oid;

  return userData;
}

bool AuthorizatorImpl::LogOut (::nprpc::flat::Span<char> session_id) {
  return authService_->logOut(std::string_view(session_id));
}

bool AuthorizatorImpl::CheckUsername (::nprpc::flat::Span<char> username) {
  return authService_->checkUsername(std::string_view(username));
}

bool AuthorizatorImpl::CheckEmail (::nprpc::flat::Span<char> email) {
  return authService_->checkEmail(std::string_view(email));
}

void AuthorizatorImpl::RegisterStepOne (::nprpc::flat::Span<char> username,
  ::nprpc::flat::Span<char> email,::nprpc::flat::Span<char> password)
{
  authService_->registerStepOne(std::string_view(username), std::string_view(email), std::string_view(password));
}

void AuthorizatorImpl::RegisterStepTwo (::nprpc::flat::Span<char> username, uint32_t code) {
  authService_->registerStepTwo(std::string_view(username), code);
}
