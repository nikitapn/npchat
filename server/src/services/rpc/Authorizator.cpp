#include "Authorizator.hpp"
#include "services/db/AuthService.hpp"

AuthorizatorImpl::AuthorizatorImpl(nprpc::Rpc& rpc, std::shared_ptr<AuthService> authService)
  : rpc_(rpc)
  , authService_(authService)
{

}

npchat::UserData AuthorizatorImpl::LogIn (::nprpc::flat::Span<char> login, ::nprpc::flat::Span<char> password) {
  return authService_->logIn(std::string_view(login), std::string_view(password));
}

npchat::UserData AuthorizatorImpl::LogInWithSessionId (::nprpc::flat::Span<char> session_id) {
  return authService_->logInWithSessionId(std::string_view(session_id));
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
  ::nprpc::flat::Span<char> email,::nprpc::flat::Span<char> password) {
  authService_->registerStepOne(std::string_view(username), std::string_view(email), std::string_view(password));
}

void AuthorizatorImpl::RegisterStepTwo (::nprpc::flat::Span<char> username, uint32_t code) {
  authService_->registerStepTwo(std::string_view(username), code);
}
